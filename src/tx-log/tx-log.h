#pragma once
/**
 * @file tx-log.h
 * @brief Per-packet TX latency logger.
 *
 * Author: Vaishnavi Balambeed
 *
 * Call TxLog_Record() immediately AFTER P1609Tx_SendWSM() returns.
 * Pass the timestamp captured BEFORE the send (t_pre_us, microseconds
 * since Unix epoch via gettimeofday).
 *
 * Fields logged to tx_log.jsonl:
 *   t_app_pre_us    - gettimeofday() stamped by caller before P1609Tx_SendWSM()
 *   t_mac_tx_us     - actual time MAC layer dispatched the frame
 *                     (from P1609TX_GetLastTx → transmitted_at, nanosecond clock)
 *   t_mac_sched_us  - time the app originally requested TX via P1609TX_RequestTx()
 *                     (from P1609TX_GetLastTx → scheduled_at)
 *   sched_delay_us  - t_mac_tx_us - t_mac_sched_us
 *                     MAC scheduling delay: time from TX request to actual dispatch
 *   mac_queue_us    - t_mac_tx_us - t_app_pre_us
 *                     total time from app pre-stamp to radio TX
 *   msg_type        - "SPAT", "MAP", "SDSM", etc.
 *   psid            - PSID from WSM header (host byte order)
 *   da              - destination MAC address
 *   data_rate_mbps  - DataRate field x 0.5 Mb/s
 *   tx_power_dbm    - TxPower field [dBm]
 *   channel         - ChannelNumber
 *   cbr_pct         - Channel busy ratio [%] from ChannelLoad header field
 *   payload_bytes   - WSM payload length [bytes]
 *   msg_cnt         - per-type sequence counter (caller maintained)
 *
 * End-to-end latency (between two GPS-disciplined OBUs):
 *   latency_ms = rx.ts_rx_ms  -  tx.t_mac_tx_us / 1000
 * Both clocks are disciplined to GPS UTC, so this is valid across units.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "dot3-wsmp.h"
#include <stdint.h>

void TxLog_Init(void);
void TxLog_Exit(void);

/**
 * @brief Log one TX packet with latency breakdown.
 *
 * @param pWSM       WSM header used for the transmission (read-only).
 * @param pMsgType   Human-readable type string ("SPAT", "MAP", "SDSM").
 * @param MsgCnt     Caller-maintained per-type sequence counter.
 * @param t_pre_us   gettimeofday() timestamp captured BEFORE P1609Tx_SendWSM(),
 *                   microseconds since Unix epoch.
 *
 * IMPORTANT: Must be called AFTER P1609Tx_SendWSM() returns, so that
 *            P1609TX_GetLastTx() returns data for THIS packet.
 */
void TxLog_Record(const struct Dot3WSMPHdr *pWSM,
                  const char               *pMsgType,
                  int                       MsgCnt,
                  uint64_t                  t_pre_us);

#ifdef __cplusplus
}
#endif
