/**
 * @file tx-log.c
 * @brief Per-packet TX logging for all message types.
 *
 * Author: Vaishnavi Balambeed
 *
 * Called from spat-tx.c, map-tx.c, sdsm-tx.c, rsa-tx.c, raw-tx.c
 * immediately before P1609Tx_SendWSM(). Stamps t_app_send_us via
 * gettimeofday() before MAC queuing begins.
 */

#include "tx-log.h"
#include "log.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
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
  if (g_tx_log_fp)
  {
    fclose(g_tx_log_fp);
    g_tx_log_fp = NULL;
  }
}

void TxLog_Record(const struct Dot3WSMPHdr *pWSM,
                  const char               *pMsgType,
                  int                       MsgCnt)
{
  if (!g_tx_log_fp || !pWSM)
    return;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t t_us = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;

  const uint8_t *da = pWSM->Tx.DA;
  uint32_t psid     = ntohl(pWSM->PSID);
  uint8_t  dataRate = pWSM->DataRate;
  int8_t   txPower  = pWSM->TxPower;
  uint8_t  chan     = pWSM->ChannelNumber;
  uint32_t cbr      = ntohl(pWSM->ChannelLoad);

  fprintf(g_tx_log_fp,
    "{"
    "\"t_app_send_us\":%" PRIu64 ","
    "\"msg_type\":\"%s\","
    "\"psid\":%u,"
    "\"da\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
    "\"data_rate_x2\":%u,"
    "\"tx_power_dbm\":%d,"
    "\"chan\":%u,"
    "\"cbr\":%u,"
    "\"msg_cnt\":%d"
    "}\n",
    t_us,
    pMsgType,
    psid,
    da[0], da[1], da[2], da[3], da[4], da[5],
    (unsigned)dataRate,
    (int)txPower,
    (unsigned)chan,
    cbr,
    MsgCnt
  );
  fflush(g_tx_log_fp);
}
