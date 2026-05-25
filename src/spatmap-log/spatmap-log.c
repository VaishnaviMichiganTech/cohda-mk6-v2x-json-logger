// spatmap-log.c
// Author: Vaishnavi Balambeed
//
// Logs SPAT and MAP messages to a JSONL file and forwards each JSON
// message over UDP simultaneously.
//
// DESIGN: Uses raw Ext WSM callback — NO SPATMAP framework, NO relevance
// filtering, NO distance check. Every received SPAT/MAP is logged immediately.
//
// Each record uses the ASN.1 JER (JSON Encoding Rules) encoder to produce a
// complete, schema-faithful JSON representation of the received message.
// Every field present in the over-the-air message is included; OPTIONAL fields
// absent from the message are omitted. No field selection or manual extraction.
//
// UDP uses IPv6 (AF_INET6) to support link-local addresses like:
//   UDP_Host = "fe80::4e2f:b578:6c6e:5b09%eth0";
//
// Add to obu.conf BEFORE the % include lines:
//   SpatMapLog:
//   {
//       UDP_Host = "fe80::4e2f:b578:6c6e:5b09%eth0";
//       UDP_Port = 5005;
//   };

#include "spatmap-log.h"

#include "asn1defs.h"
#include "dot3-wsmp.h"
#include "ext.h"
#include "id-global.h"
#include "j2735asn.h"
#include "log.h"
#include "util.h"
#include "libconfig.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SPATMAPLOG_CFG_HOST      "SpatMapLog.UDP_Host"
#define SPATMAPLOG_CFG_PORT      "SpatMapLog.UDP_Port"
#define SPATMAPLOG_DEFAULT_HOST  "::1"
#define SPATMAPLOG_DEFAULT_PORT  5005

static FILE               *g_log_fp      = NULL;
static int                 g_udp_fd      = -1;
static struct sockaddr_in6 g_udp_dest;
static int                 g_ext_handle  = -1;
static volatile int        g_initialized = 0;

static void SpatMapLog_ReadConfig(const char *pCfgFile,
                                  char       *pHostOut,
                                  size_t      hostBufLen,
                                  int        *pPortOut)
{
    strncpy(pHostOut, SPATMAPLOG_DEFAULT_HOST, hostBufLen - 1);
    pHostOut[hostBufLen - 1] = '\0';
    *pPortOut = SPATMAPLOG_DEFAULT_PORT;

    if (pCfgFile == NULL) return;

    char tmpPath[256];
    snprintf(tmpPath, sizeof(tmpPath), "/tmp/spatmaplog_%d.conf", (int)getpid());

    FILE *fin  = fopen(pCfgFile, "r");
    FILE *fout = fopen(tmpPath, "w");

    if (!fin || !fout) {
        fprintf(stderr, "SpatMapLog: cannot open %s, using defaults\n", pCfgFile);
        if (fin)  fclose(fin);
        if (fout) fclose(fout);
        return;
    }

    char line[512];
    int  in_block    = 0;
    int  brace_depth = 0;

    while (fgets(line, sizeof(line), fin)) {
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (!in_block) {
            if (strncmp(trimmed, "SpatMapLog", 10) == 0 && strchr(trimmed, ':')) {
                in_block = 1;
                brace_depth = 0;
                fputs(line, fout);
            }
        } else {
            for (char *p = line; *p; p++) {
                if (*p == '{') brace_depth++;
                if (*p == '}') brace_depth--;
            }
            fputs(line, fout);
            if (brace_depth <= 0) break;
        }
    }
    fclose(fin);
    fclose(fout);

    config_t cfg;
    config_init(&cfg);

    if (config_read_file(&cfg, tmpPath) != CONFIG_TRUE) {
        fprintf(stderr, "SpatMapLog: could not parse block (%s), using defaults\n",
                config_error_text(&cfg));
        config_destroy(&cfg);
        remove(tmpPath);
        return;
    }

    const char *host = NULL;
    if (config_lookup_string(&cfg, SPATMAPLOG_CFG_HOST, &host) == CONFIG_TRUE) {
        strncpy(pHostOut, host, hostBufLen - 1);
        pHostOut[hostBufLen - 1] = '\0';
    }
    int port = 0;
    if (config_lookup_int(&cfg, SPATMAPLOG_CFG_PORT, &port) == CONFIG_TRUE)
        *pPortOut = port;

    config_destroy(&cfg);
    remove(tmpPath);
}

/*
 * Log_JER() — encode the full decoded message via ASN.1 JER and write/forward it.
 *
 * Called for both SPAT and MAP with their respective type descriptor and decoded
 * struct pointer.  The same g_udp_fd socket and g_udp_dest address (populated at
 * init time from SpatMapLog.UDP_Host / SpatMapLog.UDP_Port in obu.conf) are used
 * for both message types — no separate sockets or config.
 *
 * File record (pretty-printed, indent=2):
 *   {
 *     "msg_type": "SPAT",
 *     "ts_rx_ms": 1234567890,
 *     "message": {
 *       "timeStamp": 262440,
 *       "intersections": [ { ... full schema ... } ]
 *     }
 *   }
 *
 * UDP datagram — sent as three iovec segments via sendmsg() with no intermediate
 * copy into a fixed buffer:
 *   segment 0: prefix  (stack string, ~60 bytes)
 *   segment 1: jer_buf (heap buffer from asn1_jer_encode2, direct reference)
 *   segment 2: suffix  (literal "\n}\n")
 *
 * jer_buf is heap-allocated by asn1_jer_encode2() with no size cap; it is freed
 * with asn1_free() after both the file write and UDP send complete.
 */
static void Log_JER(const char      *msg_type,
                    const ASN1CType *pType,
                    const void      *pDecoded,
                    uint64_t         now_ms)
{
    if (pType == NULL || pDecoded == NULL) return;

    /* --- Build the envelope prefix on the stack (~80 bytes, always fits) --- */
    char prefix[256];
    int  prefix_len = snprintf(prefix, sizeof(prefix),
        "{\n  \"msg_type\": \"%s\",\n  \"ts_rx_ms\": %" PRIu64 ",\n  \"message\": ",
        msg_type, now_ms);
    if (prefix_len < 0 || prefix_len >= (int)sizeof(prefix)) {
        fprintf(stderr, "SpatMapLog: prefix snprintf overflow\n");
        return;
    }

    static const char   suffix[]    = "\n}\n";
    static const size_t suffix_len  = sizeof(suffix) - 1; /* exclude NUL */

    /* --- JER-encode the entire decoded struct (pretty-printed, indent=2) ---
     *
     * asn1_jer_encode2() heap-allocates jer_buf and returns the byte count.
     * indent=0 means start at the top level; indent_width=2 means two spaces
     * per nesting level.  A negative return value means encoding failed.
     */
    ASN1JERParams params;
    memset(&params, 0, sizeof(params));
    params.indent       = 0;
    params.indent_width = 2;

    uint8_t      *jer_buf = NULL;
    asn1_ssize_t  jer_len = asn1_jer_encode2(&jer_buf, pType, pDecoded, &params);
    if (jer_len < 0 || jer_buf == NULL) {
        fprintf(stderr, "SpatMapLog: asn1_jer_encode2 failed for %s (ret=%zd)\n",
                msg_type, (ssize_t)jer_len);
        return;
    }

    /* --- File log: three fwrite() calls, no extra allocation --- */
    if (g_log_fp) {
        fwrite(prefix,  1, (size_t)prefix_len, g_log_fp);
        fwrite(jer_buf, 1, (size_t)jer_len,    g_log_fp);
        fwrite(suffix,  1, suffix_len,          g_log_fp);
        fflush(g_log_fp);
    }

    /* --- UDP: sendmsg() with iovec — jer_buf sent directly, zero extra copy ---
     *
     * The socket (g_udp_fd) and destination (g_udp_dest) were populated at init
     * time from SpatMapLog.UDP_Host / SpatMapLog.UDP_Port in obu.conf and are
     * shared by both SPAT and MAP messages.
     */
    if (g_udp_fd >= 0) {
        struct iovec iov[3];
        iov[0].iov_base = (void *)prefix;
        iov[0].iov_len  = (size_t)prefix_len;
        iov[1].iov_base = (void *)jer_buf;
        iov[1].iov_len  = (size_t)jer_len;
        iov[2].iov_base = (void *)suffix;
        iov[2].iov_len  = suffix_len;

        struct msghdr mhdr;
        memset(&mhdr, 0, sizeof(mhdr));
        mhdr.msg_name    = &g_udp_dest;
        mhdr.msg_namelen = sizeof(g_udp_dest);
        mhdr.msg_iov     = iov;
        mhdr.msg_iovlen  = 3;

        ssize_t sent = sendmsg(g_udp_fd, &mhdr, 0);
        if (sent < 0)
            perror("SpatMapLog: sendmsg");
    }

    /* jer_buf was allocated by the ASN.1 runtime — must use asn1_free() */
    asn1_free(jer_buf);
}

static void SpatMapLog_ExtCallback(tExtEventId  Event,
                                   tExtMessage *pMsg,
                                   void        *pPriv)
{
    (void)pPriv;
    if (Event != QSMSG_EXT_RX_WSM) return;
    if (pMsg  == NULL)              return;
    if (!g_initialized)             return;

    uint64_t now_ms = Util_Now();

    /* Type-safe check using ASN1 type descriptor */
    if (pMsg->pType == (const uintptr_t *)asn1_type_SAESPAT &&
        pMsg->pSAESPAT != NULL) {
        fprintf(stderr, "SpatMapLog: SPAT rx -- %zu intersection(s)\n",
                pMsg->pSAESPAT->intersections.count);
        Log_JER("SPAT", asn1_type_SAESPAT, pMsg->pSAESPAT, now_ms);
        return;
    }
    if (pMsg->pType == (const uintptr_t *)asn1_type_SAEMapData &&
        pMsg->pSAEMAP != NULL) {
        size_t ni = pMsg->pSAEMAP->intersections_option
                    ? pMsg->pSAEMAP->intersections.count : 0;
        fprintf(stderr, "SpatMapLog: MAP rx -- %zu intersection(s)\n", ni);
        Log_JER("MAP", asn1_type_SAEMapData, pMsg->pSAEMAP, now_ms);
        return;
    }
}

int SpatMapLog_Init(const char *pStackConfigFilename)
{
    g_initialized = 0;
    g_ext_handle  = -1;
    if (g_log_fp)      { fclose(g_log_fp);  g_log_fp = NULL; }
    if (g_udp_fd >= 0) { close(g_udp_fd);   g_udp_fd = -1;   }

    char LogDir[512], LogPath[1024];
    Log_GetLogDir(LogDir);
    snprintf(LogPath, sizeof(LogPath), "%s/spatmap_log.jsonl", LogDir);

    g_log_fp = fopen(LogPath, "w");
    if (g_log_fp == NULL) {
        fprintf(stderr, "SpatMapLog: ERROR opening %s: %s\n",
                LogPath, strerror(errno));
        return -1;
    }
    fprintf(stderr, "SpatMapLog: logging to %s\n", LogPath);

    char udp_host[64];
    int  udp_port = 0;
    SpatMapLog_ReadConfig(pStackConfigFilename, udp_host, sizeof(udp_host), &udp_port);
    fprintf(stderr, "SpatMapLog: UDP config -- host=%s port=%d\n", udp_host, udp_port);

    if (udp_port == 0) {
        fprintf(stderr, "SpatMapLog: UDP disabled\n");
        g_udp_fd = -1;
    } else {
        g_udp_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (g_udp_fd < 0) {
            fprintf(stderr, "SpatMapLog: socket() failed -- file only\n");
        } else {
            char addr_part[64]  = {0};
            char iface_part[32] = {0};
            char *pct = strchr(udp_host, '%');
            if (pct) {
                size_t alen = (size_t)(pct - udp_host);
                strncpy(addr_part,  udp_host, alen);
                strncpy(iface_part, pct + 1, sizeof(iface_part) - 1);
            } else {
                strncpy(addr_part, udp_host, sizeof(addr_part) - 1);
            }
            memset(&g_udp_dest, 0, sizeof(g_udp_dest));
            g_udp_dest.sin6_family   = AF_INET6;
            g_udp_dest.sin6_port     = htons((uint16_t)udp_port);
            g_udp_dest.sin6_scope_id = iface_part[0] ? if_nametoindex(iface_part) : 0;
            inet_pton(AF_INET6, addr_part, &g_udp_dest.sin6_addr);
            fprintf(stderr, "SpatMapLog: UDP forwarding to %s:%d\n", udp_host, udp_port);
        }
    }

    g_ext_handle = Ext_CallbackRegister(SpatMapLog_ExtCallback, NULL);
    if (g_ext_handle < 0) {
        fprintf(stderr, "SpatMapLog: Ext_CallbackRegister failed (%d)\n", g_ext_handle);
        if (g_log_fp)      { fclose(g_log_fp);  g_log_fp = NULL; }
        if (g_udp_fd >= 0) { close(g_udp_fd);   g_udp_fd = -1;   }
        return -2;
    }

    fprintf(stderr, "SpatMapLog: started (ext_handle=%d)\n", g_ext_handle);
    g_initialized = 1;
    return 0;
}

void SpatMapLog_Exit(void)
{
    g_initialized = 0;
    if (g_ext_handle >= 0) { Ext_CallbackDeregister(g_ext_handle); g_ext_handle = -1; }
    if (g_log_fp)          { fclose(g_log_fp);  g_log_fp = NULL; }
    if (g_udp_fd >= 0)     { close(g_udp_fd);   g_udp_fd = -1;   }
    fprintf(stderr, "SpatMapLog: stopped\n");
}
