/**
 * @file tx-log.c
 * @brief Per-packet TX latency logger for SPAT, MAP, SDSM and other WSM types.
 *
 * Author: Vaishnavi Balambeed
 *
 * Each call to TxLog_Record() emits one JSON line to tx_log.jsonl with:
 *
 *  t_app_pre_us    — gettimeofday() taken by the caller BEFORE P1609Tx_SendWSM()
 *  t_mac_tx_us     — actual MAC dispatch time from P1609TX_GetLastTx()
 *  t_mac_sched_us  — scheduled TX time from P1609TX_GetLastTx()
 *  sched_delay_us  — t_mac_tx_us - t_mac_sched_us  (MAC queue delay)
 *  mac_queue_us    — t_mac_tx_us - t_app_pre_us     (app-to-radio delay)
 *  + PSID, DA, data rate, TX power, channel, CBR, payload size, msg_cnt
 *
 * End-to-end latency calculation (both OBUs GPS-disciplined to UTC):
 *   latency_ms = rx_record.ts_rx_ms  −  tx_record.t_mac_tx_us / 1000
 */

#include "tx-log.h"
#include "lph.h"
#include "log.h"
#include "p1609-tx.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static FILE *g_tx_log_fp = NULL;

void TxLog_Init(void)
{
    char LogDir[512];
    char LogPath[1024];
    Log_GetLogDir(LogDir);
    snprintf(LogPath, sizeof(LogPath), "%s/tx_log.jsonl", LogDir);
    g_tx_log_fp = fopen(LogPath, "w");
    if (!g_tx_log_fp)
        fprintf(stderr, "TxLog: failed to open %s\n", LogPath);
    else
        fprintf(stderr, "TxLog: logging to %s\n", LogPath);
}

void TxLog_Exit(void)
{
    if (g_tx_log_fp) {
        fclose(g_tx_log_fp);
        g_tx_log_fp = NULL;
    }
}

void TxLog_Record(const struct Dot3WSMPHdr *pWSM,
                  const char               *pMsgType,
                  int                       MsgCnt,
                  uint64_t                  t_pre_us)
{
    if (!g_tx_log_fp || !pWSM)
        return;

    /* ------------------------------------------------------------------ */
    /* Actual MAC TX timing — P1609TX_GetLastTx() returns the transmitted  */
    /* and scheduled timespec for the most recently completed transmission. */
    /* Call this immediately after P1609Tx_SendWSM() for this packet.      */
    /* ------------------------------------------------------------------ */
    struct timespec transmitted_at = {0, 0};
    struct timespec scheduled_at   = {0, 0};
    tLPHPos pos_at_tx;
    memset(&pos_at_tx, 0, sizeof(pos_at_tx));
    P1609TX_GetLastTx(&transmitted_at, &pos_at_tx, &scheduled_at);

    /* Convert both timespec values to microseconds since Unix epoch */
    uint64_t t_mac_tx_us   = (uint64_t)transmitted_at.tv_sec * 1000000ULL
                           + (uint64_t)(transmitted_at.tv_nsec / 1000);
    uint64_t t_mac_sched_us = (uint64_t)scheduled_at.tv_sec * 1000000ULL
                            + (uint64_t)(scheduled_at.tv_nsec / 1000);

    /*
     * sched_delay_us:  Time from when the stack scheduled TX to when the MAC
     *                  actually dispatched it (accounts for DCC, EDCA contention,
     *                  and CSMA backoff).
     *
     * mac_queue_us:    Time from the app-level pre-stamp to actual radio TX.
     *                  This includes: ASN.1 encode time + mutex wait +
     *                  P1609Tx_SendWSM overhead + sched_delay.
     *
     * Both are signed to handle the (rare) case where GetLastTx returns
     * data from a previous packet if this TX failed.
     */
    int64_t sched_delay_us = (int64_t)t_mac_tx_us - (int64_t)t_mac_sched_us;
    int64_t mac_queue_us   = (int64_t)t_mac_tx_us - (int64_t)t_pre_us;

    const uint8_t *da   = pWSM->Tx.DA;
    uint32_t  psid       = ntohl(pWSM->PSID);
    uint8_t   data_rate  = pWSM->DataRate;   /* value × 0.5 = Mb/s */
    int8_t    tx_power   = pWSM->TxPower;
    uint8_t   chan       = pWSM->ChannelNumber;
    float     cbr_pct    = (float)ntohl(pWSM->ChannelLoad) * 100.0f / 65535.0f;
    uint16_t  plen       = ntohs(pWSM->Length);

    fprintf(g_tx_log_fp,
        "{"
        "\"t_app_pre_us\":%" PRIu64 ","
        "\"t_mac_tx_us\":%" PRIu64 ","
        "\"t_mac_sched_us\":%" PRIu64 ","
        "\"sched_delay_us\":%" PRId64 ","
        "\"mac_queue_us\":%" PRId64 ","
        "\"msg_type\":\"%s\","
        "\"psid\":%u,"
        "\"da\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
        "\"data_rate_mbps\":%.1f,"
        "\"tx_power_dbm\":%d,"
        "\"channel\":%u,"
        "\"cbr_pct\":%.2f,"
        "\"payload_bytes\":%u,"
        "\"msg_cnt\":%d"
        "}\n",
        t_pre_us,
        t_mac_tx_us,
        t_mac_sched_us,
        sched_delay_us,
        mac_queue_us,
        pMsgType,
        psid,
        da[0], da[1], da[2], da[3], da[4], da[5],
        (float)data_rate * 0.5f,
        (int)tx_power,
        (unsigned)chan,
        cbr_pct,
        (unsigned)plen,
        MsgCnt);

    fflush(g_tx_log_fp);
}
