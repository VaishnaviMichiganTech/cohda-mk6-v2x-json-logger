#pragma once
/**
 * @file tx-log.h
 * @brief Per-packet TX logging for all message types.
 *        Logs t_app_send_us, PSID, CBR, MCS, TxPower, channel,
 *        destination MAC, and msgCnt to tx_log.jsonl.
 *
 * Author: Vaishnavi Balambeed
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "dot3-wsmp.h"

void TxLog_Init(void);
void TxLog_Exit(void);
void TxLog_Record(const struct Dot3WSMPHdr *pWSM,
                  const char               *pMsgType,
                  int                       MsgCnt);

#ifdef __cplusplus
}
#endif
