/**
 * @file sdsm-log.c
 * @brief SDSM receiver with simultaneous file logging and UDP forwarding.
 *
 * Author: Vaishnavi Balambeed
 *
 * Each record uses the ASN.1 JER (JSON Encoding Rules) encoder to produce a
 * complete, schema-faithful JSON representation of the received SDSM message.
 * Every field present in the over-the-air message is included — detObjCommon,
 * detObjOptData (vehicle/VRU/obstacle choice), refPosXYConf, refPosElConf, etc.
 * OPTIONAL fields absent from the message are omitted. No field selection or
 * manual extraction.
 *
 * UDP datagram sent via sendmsg() iovec — jer_buf referenced directly, no copy
 * into a fixed-size intermediate buffer.
 */

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include "sdsm-log.h"

/* Cohda v2x-lib */
#include "asn1defs.h"      /* asn1_jer_encode2(), asn1_free(), ASN1JERParams   */
#include "ext.h"           /* Ext_CallbackRegister / Deregister, tExtMessage  */
#include "id-global.h"     /* QSMSG_EXT_RX_WSM                                */
#include "j2735asn.h"      /* SAESensorDataSharingMessage, SAEDetectedObject*  */
#include "log.h"           /* Log_GetLogDir()                                  */
#include "util.h"          /* Util_Now()                                       */

/* libconfig — same as spatmap-log */
#include "libconfig.h"

/* UDP socket */
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>       /* struct iovec, sendmsg()                          */

/* Standard */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/
/* Configuration constants                                                    */
/*----------------------------------------------------------------------------*/
#define SDSMLOG_CFG_HOST        "SDSMLog.UDP_Host"
#define SDSMLOG_CFG_PORT        "SDSMLog.UDP_Port"
#define SDSMLOG_DEFAULT_HOST    "127.0.0.1"
#define SDSMLOG_DEFAULT_PORT    5007

/*----------------------------------------------------------------------------*/
/* Static state                                                               */
/*----------------------------------------------------------------------------*/
static FILE              *g_log_fp   = NULL;
static int                g_udp_fd   = -1;
static struct sockaddr_in6 g_udp_dest;
static int                g_ext_handle = -1;
static volatile int       g_initialized = 0;

/*----------------------------------------------------------------------------*/
/* SDSMLog_ReadConfig: extract only the SDSMLog block, then parse            */
/*----------------------------------------------------------------------------*/
static void SDSMLog_ReadConfig(const char *pCfgFile,
                               char       *pHostOut,
                               size_t      hostBufLen,
                               int        *pPortOut)
{
    /* Defaults */
    strncpy(pHostOut, SDSMLOG_DEFAULT_HOST, hostBufLen - 1);
    pHostOut[hostBufLen - 1] = '\0';
    *pPortOut = SDSMLOG_DEFAULT_PORT;

    if (pCfgFile == NULL) return;

    /*
     * BLOCK EXTRACTION APPROACH
     *
     * The full obu.conf contains Cohda-specific syntax that libconfig
     * cannot parse even after stripping % lines:
     *
     *   ProtocolMode = 1 # comment   <- no semicolon, libconfig fails here
     *   BSMEnabled = 1               <- no semicolon
     *   Cohda_VS.VehLength = 532; 0, 10220  <- range annotation
     *
     * libconfig stops at the very first syntax error, so it never reaches
     * the SDSMLog group no matter where it appears in the file.
     *
     * Fix: scan the file line by line, copy ONLY the SDSMLog:{...}
     * block into the temp file.  The block's own lines are well-formed
     * libconfig syntax, so parsing always succeeds.
     */
    char tmpPath[256];
    snprintf(tmpPath, sizeof(tmpPath),
             "/tmp/sdsmlog_%d.conf", (int)getpid());

    FILE *fin  = fopen(pCfgFile, "r");
    FILE *fout = fopen(tmpPath,  "w");

    if (!fin || !fout) {
        fprintf(stderr,
                "SDSMLog: cannot open %s for config extraction, "
                "using defaults\n", pCfgFile);
        if (fin)  fclose(fin);
        if (fout) fclose(fout);
        return;
    }

    char line[512];
    int  in_block    = 0;   /* 1 once we have seen "SDSMLog:" */
    int  brace_depth = 0;

    while (fgets(line, sizeof(line), fin)) {
        /* Trim leading whitespace for the match check only */
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        if (!in_block) {
            /*
             * Look for the group header line: starts with "SDSMLog"
             * and contains ":" — e.g. "SDSMLog:" or "SDSMLog :"
             */
            if (strncmp(trimmed, "SDSMLog", 7) == 0) {
                char *colon = strchr(trimmed, ':');
                if (colon) {
                    in_block = 1;
                    brace_depth = 0;
                    fputs(line, fout);   /* write "SDSMLog:" verbatim */
                }
            }
        } else {
            /* Count braces to know when the block ends */
            for (char *p = line; *p; p++) {
                if (*p == '{') brace_depth++;
                if (*p == '}') brace_depth--;
            }
            fputs(line, fout);   /* write block contents verbatim */
            if (brace_depth <= 0) break;   /* closing "};" reached */
        }
    }

    fclose(fin);
    fclose(fout);

    config_t cfg;
    config_init(&cfg);

    if (config_read_file(&cfg, tmpPath) != CONFIG_TRUE) {
        fprintf(stderr,
                "SDSMLog: could not parse extracted SDSMLog block "
                "(%s), using defaults\n",
                config_error_text(&cfg));
        config_destroy(&cfg);
        remove(tmpPath);
        return;
    }

    /* UDP_Host */
    const char *host = NULL;
    if (config_lookup_string(&cfg, SDSMLOG_CFG_HOST, &host) == CONFIG_TRUE) {
        strncpy(pHostOut, host, hostBufLen - 1);
        pHostOut[hostBufLen - 1] = '\0';
    }

    /* UDP_Port */
    int port = 0;
    if (config_lookup_int(&cfg, SDSMLOG_CFG_PORT, &port) == CONFIG_TRUE) {
        *pPortOut = port;
    }

    config_destroy(&cfg);
    remove(tmpPath);
}

/*----------------------------------------------------------------------------*/
/* Log_JER — JER-encode the full decoded struct and write/forward it         */
/*----------------------------------------------------------------------------*/
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
        fprintf(stderr, "SDSMLog: asn1_jer_encode2 failed for %s (ret=%zd)\n",
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
            perror("SDSMLog: sendmsg");
    }

    asn1_free(jer_buf);
}

/*----------------------------------------------------------------------------*/
/* Ext callback — fires for every received WSM                                */
/*----------------------------------------------------------------------------*/
static void SDSMLog_ExtCallback(tExtEventId Event,
                                tExtMessage *pMsg,
                                void        *pPriv)
{
    (void)pPriv;

    if (Event != QSMSG_EXT_RX_WSM) return;
    if (pMsg  == NULL)              return;

    if (pMsg->pType != (const uintptr_t *)asn1_type_SAESensorDataSharingMessage) return;
    const struct SAESensorDataSharingMessage *pSDSM = pMsg->pSAESDSM;
    if (pSDSM == NULL)   return;
    if (!g_initialized)  return;

    /* Skip corrupt/uninitialized packets */
    if (pSDSM->sourceID.len == 0 &&
        pSDSM->refPos.lat   == 0 &&
        pSDSM->refPos.Long  == 0) return;
    if (pSDSM->objects.count > 255) return;

    uint64_t now_ms = Util_Now();
    fprintf(stderr, "SDSMLog: SDSM rx -- %zu object(s)\n", pSDSM->objects.count);
    Log_JER("SDSM", asn1_type_SAESensorDataSharingMessage, pSDSM, now_ms);
}

/*----------------------------------------------------------------------------*/
/* SDSMLog_Init                                                               */
/*----------------------------------------------------------------------------*/
int SDSMLog_Init(const char *pStackConfigFilename)
{
    /* Safe reset — do NOT call Ext_CallbackDeregister here */
    g_initialized = 0;
    g_ext_handle  = -1;
    if (g_log_fp) { fclose(g_log_fp); g_log_fp = NULL; }
    if (g_udp_fd >= 0) { close(g_udp_fd); g_udp_fd = -1; }
    /* Open log file */
    char LogDir[512];
    char LogPath[1024];
    Log_GetLogDir(LogDir);
    snprintf(LogPath, sizeof(LogPath), "%s/sdsm_log.jsonl", LogDir);

    g_log_fp = fopen(LogPath, "w");
    if (g_log_fp == NULL) {
        fprintf(stderr, "SDSMLog: failed to open log file %s: %s\n",
                LogPath, strerror(errno));
        return -1;
    }
    fprintf(stderr, "SDSMLog: logging to %s\n", LogPath);

    /* Read UDP config from stack conf file */
    char udp_host[64];
    int  udp_port = 0;
    SDSMLog_ReadConfig(pStackConfigFilename,
                       udp_host, sizeof(udp_host),
                       &udp_port);

    fprintf(stderr, "SDSMLog: UDP config - host=%s port=%d\n",
            udp_host, udp_port);

    /* Open UDP socket (skip if port == 0) */
    if (udp_port == 0) {
        fprintf(stderr, "SDSMLog: UDP_Port=0, UDP forwarding disabled\n");
        g_udp_fd = -1;
    } else {
        g_udp_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (g_udp_fd >= 0) {
            char addr_part[64] = {0};
            char iface_part[32] = {0};
            char *pct = strchr(udp_host, '%');
            if (pct) {
                size_t alen = (size_t)(pct - udp_host);
                strncpy(addr_part, udp_host, alen);
                strncpy(iface_part, pct + 1, sizeof(iface_part) - 1);
            } else {
                strncpy(addr_part, udp_host, sizeof(addr_part) - 1);
            }
            memset(&g_udp_dest, 0, sizeof(g_udp_dest));
            g_udp_dest.sin6_family   = AF_INET6;
            g_udp_dest.sin6_port     = htons((uint16_t)udp_port);
            g_udp_dest.sin6_scope_id = iface_part[0] ? if_nametoindex(iface_part) : 0;
            inet_pton(AF_INET6, addr_part, &g_udp_dest.sin6_addr);
            fprintf(stderr, "SDSMLog: UDP forwarding to %s:%d\n",
                    udp_host, udp_port);
        } else {
            fprintf(stderr, "SDSMLog: socket() failed: %s\n", strerror(errno));
        }
    }

    /* Register Ext callback */
    g_ext_handle = Ext_CallbackRegister(SDSMLog_ExtCallback, NULL);
    if (g_ext_handle < 0) {
        fprintf(stderr, "SDSMLog: Ext_CallbackRegister failed (%d)\n",
                g_ext_handle);
        if (g_log_fp) { fclose(g_log_fp); g_log_fp = NULL; }
        if (g_udp_fd >= 0) { close(g_udp_fd); g_udp_fd = -1; }
        return -1;
    }

    fprintf(stderr, "SDSMLog: started (ext_handle=%d)\n", g_ext_handle);
    g_initialized = 1;
    return 0;
}

/*----------------------------------------------------------------------------*/
/* SDSMLog_Exit                                                               */
/*----------------------------------------------------------------------------*/
void SDSMLog_Exit(void)
{
    g_initialized = 0;
    if (g_ext_handle >= 0) {
        Ext_CallbackDeregister(g_ext_handle);
        g_ext_handle = -1;
    }
    if (g_log_fp) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    if (g_udp_fd >= 0) {
        close(g_udp_fd);
        g_udp_fd = -1;
    }
    fprintf(stderr, "SDSMLog: stopped\n");
}
