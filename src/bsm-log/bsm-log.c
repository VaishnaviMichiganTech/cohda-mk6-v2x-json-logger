// bsm-log.c
// Author: Vaishnavi Balambeed
//
// Logs BSM (Basic Safety Message / J2735) messages to a JSONL file and
// forwards each JSON message over UDP simultaneously.
//
// DESIGN: Uses raw Ext WSM callback — NO OBERx framework, NO relevance
// filtering, NO distance check. Every received BSM is logged immediately.
//
// Each record uses the ASN.1 JER (JSON Encoding Rules) encoder to produce a
// complete, schema-faithful JSON representation of the received BSM — all 14
// core data fields plus the full Part II Vehicle Safety Extensions (path
// history, path prediction, roll/pitch/yaw, event indicators) when present.
// OPTIONAL fields absent from the message are omitted.
//
// UDP uses IPv6 (AF_INET6) to support link-local addresses like:
//   UDP_Host = "fe80::4e2f:b578:6c6e:5b09%eth0";
//
// Add to obu.conf BEFORE the % include lines:
//   BSMLog:
//   {
//       UDP_Host = "127.0.0.1";
//       UDP_Port = 5008;
//   };

#include "bsm-log.h"

#include "asn1defs.h"
#include "J2735_BSM.h"
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

#define BSMLOG_CFG_HOST      "BSMLog.UDP_Host"
#define BSMLOG_CFG_PORT      "BSMLog.UDP_Port"
#define BSMLOG_DEFAULT_HOST  "127.0.0.1"
#define BSMLOG_DEFAULT_PORT  5008

static FILE               *g_log_fp      = NULL;
static int                 g_udp_fd      = -1;
static struct sockaddr_in6 g_udp_dest;
static int                 g_ext_handle  = -1;
static volatile int        g_initialized = 0;

// -----------------------------------------------------------------------
// Config: block-extraction approach
//
// obu.conf contains Cohda-specific syntax that libconfig cannot parse:
//   - assignments without semicolons
//   - # comments without semicolons
//   - range annotations like "532; 0, 10220"
//
// We scan the file line-by-line and copy only the BSMLog { ... } block
// to a temp file, then hand that temp file to libconfig. This avoids
// every non-standard line that appears outside our block.
// -----------------------------------------------------------------------
static void BSMLog_ReadConfig(const char *pCfgFile,
                              char       *pHostOut,
                              size_t      hostBufLen,
                              int        *pPortOut)
{
    strncpy(pHostOut, BSMLOG_DEFAULT_HOST, hostBufLen - 1);
    pHostOut[hostBufLen - 1] = '\0';
    *pPortOut = BSMLOG_DEFAULT_PORT;

    if (pCfgFile == NULL) return;

    char tmpPath[256];
    snprintf(tmpPath, sizeof(tmpPath), "/tmp/bsmlog_%d.conf", (int)getpid());

    FILE *fin  = fopen(pCfgFile, "r");
    FILE *fout = fopen(tmpPath, "w");

    if (!fin || !fout) {
        fprintf(stderr, "BSMLog: cannot open %s, using defaults\n", pCfgFile);
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
            if (strncmp(trimmed, "BSMLog", 6) == 0 && strchr(trimmed, ':')) {
                in_block    = 1;
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
        fprintf(stderr, "BSMLog: could not parse block (%s), using defaults\n",
                config_error_text(&cfg));
        config_destroy(&cfg);
        remove(tmpPath);
        return;
    }

    const char *host = NULL;
    if (config_lookup_string(&cfg, BSMLOG_CFG_HOST, &host) == CONFIG_TRUE) {
        strncpy(pHostOut, host, hostBufLen - 1);
        pHostOut[hostBufLen - 1] = '\0';
    }
    int port = 0;
    if (config_lookup_int(&cfg, BSMLOG_CFG_PORT, &port) == CONFIG_TRUE)
        *pPortOut = port;

    config_destroy(&cfg);
    remove(tmpPath);
}

static void Log_JER(const char      *msg_type,
                    const ASN1CType *pType,
                    const void      *pDecoded,
                    uint64_t         now_ms)
{
    if (pType == NULL || pDecoded == NULL) return;

    char prefix[256];
    int  prefix_len = snprintf(prefix, sizeof(prefix),
        "{\n  \"msg_type\": \"%s\",\n  \"ts_rx_ms\": %" PRIu64 ",\n  \"message\": ",
        msg_type, now_ms);
    if (prefix_len < 0 || prefix_len >= (int)sizeof(prefix)) return;

    static const char   suffix[]   = "\n}\n";
    static const size_t suffix_len = sizeof(suffix) - 1;

    ASN1JERParams params;
    memset(&params, 0, sizeof(params));
    params.indent       = 0;
    params.indent_width = 2;

    uint8_t      *jer_buf = NULL;
    asn1_ssize_t  jer_len = asn1_jer_encode2(&jer_buf, pType, pDecoded, &params);
    if (jer_len < 0 || jer_buf == NULL) {
        fprintf(stderr, "BSMLog: asn1_jer_encode2 failed for %s (ret=%zd)\n",
                msg_type, (ssize_t)jer_len);
        return;
    }

    if (g_log_fp) {
        fwrite(prefix,  1, (size_t)prefix_len, g_log_fp);
        fwrite(jer_buf, 1, (size_t)jer_len,    g_log_fp);
        fwrite(suffix,  1, suffix_len,          g_log_fp);
        fflush(g_log_fp);
    }

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

        if (sendmsg(g_udp_fd, &mhdr, 0) < 0)
            perror("BSMLog: sendmsg");
    }

    asn1_free(jer_buf);
}

static void BSMLog_ExtCallback(tExtEventId  Event,
                               tExtMessage *pMsg,
                               void        *pPriv)
{
    (void)pPriv;
    if (Event != QSMSG_EXT_RX_WSM) return;
    if (pMsg  == NULL)              return;
    if (!g_initialized)             return;
    if (pMsg->pType != (const uintptr_t *)asn1_type_SAEBasicSafetyMessage) return;
    if (pMsg->pBSM == NULL)         return;

    uint64_t now_ms = Util_Now();

    fprintf(stderr, "BSMLog: BSM rx -- msgCnt=%d lat=%.6f lon=%.6f spd=%.2f m/s\n",
            (int)pMsg->pBSM->coreData.msgCnt,
            pMsg->pBSM->coreData.lat  * 1e-7,
            pMsg->pBSM->coreData.Long * 1e-7,
            pMsg->pBSM->coreData.speed * 0.02);

    Log_JER("BSM", asn1_type_SAEBasicSafetyMessage, pMsg->pBSM, now_ms);
}

int BSMLog_Init(const char *pStackConfigFilename)
{
    g_initialized = 0;
    g_ext_handle  = -1;
    if (g_log_fp)      { fclose(g_log_fp);  g_log_fp = NULL; }
    if (g_udp_fd >= 0) { close(g_udp_fd);   g_udp_fd = -1;   }

    char LogDir[512], LogPath[1024];
    Log_GetLogDir(LogDir);
    snprintf(LogPath, sizeof(LogPath), "%s/bsm_log.jsonl", LogDir);

    g_log_fp = fopen(LogPath, "w");
    if (g_log_fp == NULL) {
        fprintf(stderr, "BSMLog: ERROR opening %s: %s\n",
                LogPath, strerror(errno));
        return -1;
    }
    fprintf(stderr, "BSMLog: logging to %s\n", LogPath);

    char udp_host[64];
    int  udp_port = 0;
    BSMLog_ReadConfig(pStackConfigFilename, udp_host, sizeof(udp_host), &udp_port);
    fprintf(stderr, "BSMLog: UDP config -- host=%s port=%d\n", udp_host, udp_port);

    if (udp_port == 0) {
        fprintf(stderr, "BSMLog: UDP disabled\n");
        g_udp_fd = -1;
    } else {
        g_udp_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (g_udp_fd < 0) {
            fprintf(stderr, "BSMLog: socket() failed -- file only\n");
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

            /* "::ffff:" (7) + up to 15 chars for IPv4 + NUL = 23; extra room for IPv6 */
            char mapped[80] = {0};
            if (strchr(addr_part, ':') == NULL)
                snprintf(mapped, sizeof(mapped), "::ffff:%s", addr_part);
            else
                strncpy(mapped, addr_part, sizeof(mapped) - 1);

            memset(&g_udp_dest, 0, sizeof(g_udp_dest));
            g_udp_dest.sin6_family   = AF_INET6;
            g_udp_dest.sin6_port     = htons((uint16_t)udp_port);
            g_udp_dest.sin6_scope_id = iface_part[0] ? if_nametoindex(iface_part) : 0;
            if (inet_pton(AF_INET6, mapped, &g_udp_dest.sin6_addr) != 1) {
                fprintf(stderr, "BSMLog: inet_pton failed for %s -- UDP disabled\n", mapped);
                close(g_udp_fd);
                g_udp_fd = -1;
            } else {
                fprintf(stderr, "BSMLog: UDP forwarding to %s:%d\n", udp_host, udp_port);
            }
        }
    }

    g_ext_handle = Ext_CallbackRegister(BSMLog_ExtCallback, NULL);
    if (g_ext_handle < 0) {
        fprintf(stderr, "BSMLog: Ext_CallbackRegister failed (%d)\n", g_ext_handle);
        if (g_log_fp)      { fclose(g_log_fp);  g_log_fp = NULL; }
        if (g_udp_fd >= 0) { close(g_udp_fd);   g_udp_fd = -1;   }
        return -2;
    }

    fprintf(stderr, "BSMLog: started (ext_handle=%d)\n", g_ext_handle);
    g_initialized = 1;
    return 0;
}

void BSMLog_Exit(void)
{
    g_initialized = 0;
    if (g_ext_handle >= 0) { Ext_CallbackDeregister(g_ext_handle); g_ext_handle = -1; }
    if (g_log_fp)          { fclose(g_log_fp);  g_log_fp = NULL; }
    if (g_udp_fd >= 0)     { close(g_udp_fd);   g_udp_fd = -1;   }
    fprintf(stderr, "BSMLog: stopped\n");
}
