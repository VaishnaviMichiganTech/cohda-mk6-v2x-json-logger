/**
 * @addtogroup app_ntrip Ntrip client to DSRC forwarder
 * @{
 *
 * @dontinclude Ntrip-rx.c
 */

//------------------------------------------------------------------------------
// Copyright (c) 2019 Cohda Wireless Pty Ltd
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "Ntrip-rx.h"
#include "Ntrip.h"

#include "debug-levels.h"
#include "dot3-wsmp.h"
#include "etsi-rtcm-if.h"
#include "id-global.h"
#include "lph.h"
#include "p1609-tx-msg.h"
#include "pcap-log.h"
#include "tcp.h"
#include "util.h"

#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "conf2.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

#define D_LEVEL Ntrip_Cust_App_Ntrip_DebugLevel

#define NTRIP_WSMP_EXPIRY_TIME     (0)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/// Internal thread state values
typedef enum
{
  NTRIP_STATE_NONE = 0,
  NTRIP_STATE_INIT,
  NTRIP_STATE_OPEN,
  NTRIP_STATE_GET,
  NTRIP_STATE_200_OK,
  NTRIP_STATE_STREAM,
  NTRIP_STATE_EXIT,
} eNtripRxState;

/// RTCM message item
typedef struct RTCMMsg
{
  const char *pBuf;
  int Len;
} tRTCMMsg;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local (static) Function Prototypes
//------------------------------------------------------------------------------

// Weak symbols defined to allow single compilation of Ntrip plugin to be
// used in different applciations (with different Stack types)

tRTCMStatusCode ETSIRTCM_SendRTCM(const tRTCMMgmt *pMgmt,
                                  const ITSRTCMcorrections *pRtcm) __attribute__((weak));

int P1609Tx_SendWSM(const struct Dot3WSMPHdr *pWSM,
                    bool SignMessage,
                    tP1609TxCertAttach CertAttach,
                    int LogFd) __attribute__((weak));

extern const ASN1CType __attribute__((weak)) asn1_type_SAEMessageFrame[];
extern const ASN1CType __attribute__((weak)) asn1_type_SAERTCMcorrections[];
extern const ASN1CType __attribute__((weak)) asn1_type_ITSRTCMcorrections[];

//------------------------------------------------------------------------------
// Base 64 encoding support from: https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
//------------------------------------------------------------------------------

static const unsigned char base64_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
static unsigned char * base64_encode(const unsigned char *src,
                                     size_t len,
                                     size_t *out_len)
{
  unsigned char *out, *pos;
  const unsigned char *end, *in;
  size_t olen;
  int line_len;

  olen = len * 4 / 3 + 4; // 3-byte blocks to 4-byte
  olen += olen / 72; // line feeds
  olen++; // nul termination
  if (olen < len)
    return NULL; // integer overflow
  out = malloc(olen);
  if (out == NULL)
    return NULL;

  end = src + len;
  in = src;
  pos = out;
  line_len = 0;
  while (end - in >= 3) {
    *pos++ = base64_table[in[0] >> 2];
    *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
    *pos++ = base64_table[in[2] & 0x3f];
    in += 3;
    line_len += 4;
    if (0) { // (line_len >= 72) {
      *pos++ = '\n';
      line_len = 0;
    }
  }

  if (end - in) {
    *pos++ = base64_table[in[0] >> 2];
    if (end - in == 1) {
      *pos++ = base64_table[(in[0] & 0x03) << 4];
      *pos++ = '=';
    } else {
      *pos++ = base64_table[((in[0] & 0x03) << 4) |
                (in[1] >> 4)];
      *pos++ = base64_table[(in[1] & 0x0f) << 2];
    }
    *pos++ = '=';
    line_len += 4;
  }

  (void)line_len;
  if (0) // (line_len)
    *pos++ = '\n';

  *pos = '\0';
  if (out_len)
    *out_len = pos - out;
  return out;
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

//  Convert decimal degrees to degrees, minutes and seconds
static double NtripRx_DegreesMinutes(double DecimalDegrees)
{
  bool Negative = false;
  if (DecimalDegrees < 0)
  {
    Negative = true;
    DecimalDegrees = -DecimalDegrees;
  }

  int Degrees = (int)DecimalDegrees;
  double Fraction = DecimalDegrees - (double)Degrees;
  double Minutes = Fraction*60;
  double DM = (Degrees * 100) + (Minutes);

  if (Negative)
  {
    DM = -DM;
  }
  return DM;
}

// Get current position info from LPH
static int NtripRx_LPHPos(struct LPHPos *pLPHPos)
{
  int Res;
  void *pLPH = NULL;
  const struct LPHData *pLPHData = NULL;

  pLPH = LPH_Open();
  if (pLPH == NULL)
  {
    d_printf(D_WARN, NULL, "LPH_Open() = NULL\n");
    return -EPIPE;
  }

  Res = LPH_Data(pLPH, &pLPHData);
  if ((Res == 0) && (pLPHData != NULL))
  {
    memcpy(pLPHPos, &(pLPHData->Pos), sizeof(*pLPHPos));
  }
  else
  {
    d_printf(D_WARN, NULL, "LPH_Data() = %d\n", Res);
    Res = -ENODATA;
  }

  LPH_Close(pLPH);

  return Res;
}

int NtripRx_GetGGA(char *pBuf, size_t BufLen)
{
  int Res;

  // Get current position info from LPH
  struct LPHPos LPHPos = { 0, };
  Res = NtripRx_LPHPos(&LPHPos);
  if ((Res != 0) || (LPHPos.Mode < 2))
  {
    d_printf(D_WARN, NULL, "NtripRx_LPHPos() = %d\n", Res);
    goto Error;
  }

  // Get the timestamp from the LPH (corresponding to when the pos info was taken)
  struct tm DateAndTime;
  if (!gmtime_r(&(LPHPos.Time.tv_sec), &DateAndTime))
  {
    Res = -errno;
    goto Error;
  }

  double Lat_dms = NtripRx_DegreesMinutes(1e-7 * LPHPos.Latitude);
  double Lon_dms = NtripRx_DegreesMinutes(1e-7 * LPHPos.Longitude);
  Res = snprintf(pBuf, BufLen-1,
                 "$GPGGA,%02d%02d%02d.%02d,%07.4f,%c,%07.4f,%c,%1d,%02d,%03.1f,%03.1f,%c,%03.1f,%c,,*%02x",
                 DateAndTime.tm_hour,
                 DateAndTime.tm_min,
                 DateAndTime.tm_sec,
                 (int)(LPHPos.Time.tv_usec / 10000),
                 fabs(Lat_dms),
                 (Lat_dms > 0) ? 'N' : 'S',
                 fabs(Lon_dms),
                 (Lon_dms > 0) ? 'E' : 'W',
                 LPHPos.Mode,
                 LPHPos.NbrSatellitesUsed,
                 0.9f, // HDOP
                 1e-1 * LPHPos.Elevation, // MSL (not really)
                 'M',
                 0.0f, // WG84 <-> MSL seperation
                 'M',
                 0x00); // CRC byte placeholder
  if (Res < 0)
  {
    Res = -errno;
    d_printf(D_WARN, NULL, "snprintf(GPGGA...) = %d\n", Res);
    goto Error;
  }
  pBuf[BufLen-1] = '\0';
  Res = strlen(pBuf);

  // Calculate & replace the NMEA CRC byte
  {
    int i;
    int Start = 0;
    int End = 0;
    int Len = Res;

    for (i = 0; i < Len; i++)
    {
      if (pBuf[i] == '$')
        Start = i;
      if (pBuf[i] == '*')
        End = i;
    }
    if (End > Start)
    {
      // XOR every character between '$' and '*'
      uint8_t CRC = 0;
      for (i = Start+1; i < End; i++)
      {
        CRC = CRC ^ (uint8_t)pBuf[i];
      }
      // We know it fits due to the prior snprintf
      (void)sprintf(pBuf + End + 1, "%02x", CRC);
    }
  }
  d_printf(D_INFO, NULL, "LPH: '%s'\n", pBuf);

Error:
  d_printf(D_TST, NULL, "%d: %s\n", Res, pBuf);
  return Res;
}

/**
 * @brief Parse the provided URI
 * @param  pURI e.g. "ntrip://username:password^ntrip.example.com:80/mountpoint"
 * @return      Zero for success or a negative errno
 */
int NtripRx_ParseURI(const char *pURI)
{
  int Res;

  d_fnstart(D_TST, NULL, "(%p)\n", pURI);

  // Clear the decks
  memset(Ntrip.Rx.Credentials, 0, sizeof(Ntrip.Rx.Credentials));
  memset(Ntrip.Rx.Hostname,    0, sizeof(Ntrip.Rx.Hostname));
  memset(Ntrip.Rx.Port,        0, sizeof(Ntrip.Rx.Port));
  memset(Ntrip.Rx.Mountpoint,  0, sizeof(Ntrip.Rx.Mountpoint));

  if (pURI == NULL)
  {
    Res = -EFAULT;
    goto Error;
  }

  // Work on our own 'rw' copy
  char Tmp[1024] = { 0, };
  char *pTmp = Tmp;
  d_assert(strlen(pURI) < sizeof(Tmp));
  (void)strncpy(Tmp, pURI, sizeof(Tmp)-1);
  d_printf(D_TST, NULL, "URI '%s'\n", Tmp);

  // Strip off the "ntrip://" prefix
  pTmp = strstr(pTmp, "ntrip://");
  if (pTmp == NULL)
  {
    d_printf(D_WARN, NULL, "Can't find 'ntrip://' prefix in '%s'\n", Tmp);
    Res = -EFAULT;
    goto Error;
  }
  pTmp += strlen("ntrip://");

  char *pCaret;
  char *pColon;
  char *pSlash;
  char *pAuth = NULL;
  char *pHost = NULL;
  char *pPort = NULL;
  char *pMount = NULL;

  // Does it include the username & password?
  pCaret = strchr(pTmp, '^');
  if (pCaret != NULL)
  {
    pColon = strchr(pTmp, ':');
    if ((pColon != NULL) && (pColon < pCaret))
    {
      pAuth = pTmp;
      *pCaret = '\0';
      pTmp = pCaret + 1;
    }
    else
    {
      d_printf(D_WARN, NULL, "Can't extract 'user:password^' from '%s'\n", Tmp);
      Res = -EFAULT;
      goto Error;
    }
  }
  pHost = pTmp;

  // Gotta have a mount point after the /
  pSlash = strchr(pHost, '/');
  if (pSlash != NULL)
  {
    *pSlash = '\0';
    pMount = pSlash + 1;
  }
  else
  {
    d_printf(D_WARN, NULL, "Can't extract '/mountpoint' from '%s'\n", Tmp);
    Res = -EFAULT;
    goto Error;
  }

  // Extract or lookup the optional port number
  pColon = strchr(pHost, ':');
  if (pColon != NULL)
  {
    pPort = pColon + 1;
    *pColon = '\0';
  }
  if (pPort == NULL)
  {
    pPort = "2101";
  }
  d_assert(pPort != NULL);

  // Copy out the parsed strings
  if (pAuth != NULL)
  {
    d_printf(D_TST, NULL, "Auth '%s'\n", pAuth);
    d_assert(strlen(pAuth) < sizeof(Ntrip.Rx.Credentials));
    (void)strncpy(Ntrip.Rx.Credentials,
                  pAuth,
                  sizeof(Ntrip.Rx.Credentials)-1);
    Ntrip.Rx.Credentials[sizeof(Ntrip.Rx.Credentials)-1] = '\0';
  }

  {
    d_printf(D_TST, NULL, "Host '%s'\n", pHost);
    d_assert(strlen(pHost) < sizeof(Ntrip.Rx.Hostname));
    (void)strncpy(Ntrip.Rx.Hostname,
                  pHost,
                  sizeof(Ntrip.Rx.Hostname)-1);
    Ntrip.Rx.Hostname[sizeof(Ntrip.Rx.Hostname)-1] = '\0';
  }

  {
    d_printf(D_TST, NULL, "Port '%s'\n", pPort);
    d_assert(strlen(pPort) < sizeof(Ntrip.Rx.Port));
    (void)strncpy(Ntrip.Rx.Port,
                  pPort,
                  sizeof(Ntrip.Rx.Port)-1);
    Ntrip.Rx.Port[sizeof(Ntrip.Rx.Port)-1] = '\0';
  }

  {
    d_printf(D_TST, NULL, "Mount '%s'\n", pMount);
    d_assert(strlen(pMount) < sizeof(Ntrip.Rx.Mountpoint));
    (void)strncpy(Ntrip.Rx.Mountpoint,
                  pMount,
                  sizeof(Ntrip.Rx.Mountpoint)-1);
    Ntrip.Rx.Mountpoint[sizeof(Ntrip.Rx.Mountpoint)-1] = '\0';
  }
  Res = 0;

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}


static void NtripRx_SendCN(int Items, const tRTCMMsg *pMsgs)
{
  (void)pMsgs;
  (void)Items;
  return; // TODO
}

static void NtripRx_SendEU(int Items, const tRTCMMsg *pMsgs)
{
  static int msgCnt = 0;

  // Send RTCM over ETSIRTCM
  // Fill out ETSI RTCM packet for sending using the ETSI RTCM API:
  tRTCMMgmt RTCMMgmt = {0};
  ITSRTCMcorrections *pRTCMData = asn1_mallocz(sizeof(ITSRTCMcorrections));
  if (pRTCMData != NULL)
  {
    pRTCMData->msgs.count = Items;
    pRTCMData->msgs.tab = (ITSRTCMmessage *)
      asn1_mallocz(pRTCMData->msgs.count * sizeof(ITSRTCMmessage));
    if (pRTCMData->msgs.tab == NULL)
    {
      d_printf(D_WARN, NULL, "Unable to allocate ITSRTCMmessage (%p)\n",
               pRTCMData->msgs.tab);
      goto RTCMFree;
    }
    int Index;
    for (Index = 0; Index < Items; Index++)
    {
      pRTCMData->msgs.tab[Index].len = pMsgs[Index].Len;
      pRTCMData->msgs.tab[Index].buf = asn1_mallocz(pRTCMData->msgs.tab[Index].len);
      if (pRTCMData->msgs.tab[Index].buf == NULL)
      {
        d_printf(D_WARN, NULL, "Unable to allocate RTCMMessage (%p)\n",
                 pRTCMData->msgs.tab);
        goto RTCMFree;
      }
      else
      {
        memcpy(pRTCMData->msgs.tab[Index].buf, pMsgs[Index].pBuf, pRTCMData->msgs.tab[Index].len);
      }
    }

    if (msgCnt > 127)
    {
      msgCnt = 0;
    }
    pRTCMData->msgCnt = msgCnt++;
    // TODO: Check the RTCM version we are using
    pRTCMData->rev = Ntrip.Param.RTCM_Revision;
    // TODO: Check additional optional items
    // ISO 19091 provides no clarity on this
    pRTCMData->timeStamp_option = false;
    pRTCMData->anchorPoint_option = false;
    pRTCMData->rtcmHeader_option = false;
    pRTCMData->regional_option = false;

    RTCMMgmt.PktTransport = ETSIFAC_GN_TRANSPORT_SHB;
    RTCMMgmt.CommsProfile = ETSIFAC_GN_PROFILE_DEFAULT;
    RTCMMgmt.HopLimit = Ntrip.Param.HopLimit;
    RTCMMgmt.TrafficClass = ETSIFAC_GN_TC_ITS_DP3;

    int Res = ETSIRTCM_SendRTCM(&RTCMMgmt, pRTCMData);
    if (Res != ETSIRTCM_SUCCESS)
    {
      d_printf(D_WARN, NULL, "ETSIRTCM_SendRTCM() = %d\n", Res);
    }
    // Sent RTCM over DSRC
  }
  else
  {
    d_printf(D_WARN, NULL, "Unable to allocate ITSRTCMcorrections (%p)\n",
             pRTCMData);
  }

RTCMFree:
  if (pRTCMData != NULL)
  {
    asn1_free_value(asn1_type_ITSRTCMcorrections, pRTCMData);
  }
}

static void NtripRx_SendNA(int Items, const tRTCMMsg *pMsgs)
{
  SAERTCMcorrections *pRTCMData = asn1_mallocz(sizeof(SAERTCMcorrections));
  if (pRTCMData != NULL)
  {
    static int msgCnt = 0;
    msgCnt += 1;
    if (msgCnt > 127)
    {
      msgCnt = 0;
    }
    pRTCMData->msgCnt = msgCnt;
    pRTCMData->rev = SAERTCM_Revision_rtcmRev3;
    pRTCMData->timeStamp_option = false;
    pRTCMData->anchorPoint_option = false;
    pRTCMData->rtcmHeader_option = false;
    pRTCMData->regional_option = false;

    pRTCMData->msgs.count = Items;
    pRTCMData->msgs.tab = (SAERTCMmessage *)
      asn1_mallocz(pRTCMData->msgs.count * sizeof(SAERTCMmessage));
    if (pRTCMData->msgs.tab == NULL)
    {
      d_printf(D_WARN, NULL, "Unable to allocate for ITSRTCMmessage (%p)\n",
               pRTCMData->msgs.tab);
      pRTCMData->msgs.count = 0;
    }
    else
    {
      int Index;
      for (Index = 0; Index < Items; Index++)
      {
        pRTCMData->msgs.tab[Index].len = pMsgs[Index].Len;
        pRTCMData->msgs.tab[Index].buf = asn1_mallocz(pRTCMData->msgs.tab[Index].len);
        if (pRTCMData->msgs.tab[Index].buf == NULL)
        {
          d_printf(D_WARN, NULL, "Unable to allocate RTCMMessage (%p)\n",
                   pRTCMData->msgs.tab);
          pRTCMData->msgs.tab[Index].len = 0;
        }
        else
        {
          memcpy(pRTCMData->msgs.tab[Index].buf, pMsgs[Index].pBuf, pRTCMData->msgs.tab[Index].len);
        }
      }
    }

    // Encode as UPER
    SAEMessageFrame Msg;
    Msg.messageId = SAEDSRCmsgID_rtcmCorrections;
    const ASN1CType *pType = asn1_type_SAERTCMcorrections;
    Msg.value.type = (ASN1CType *)pType;
    Msg.value.u.data = (void *)pRTCMData;

    uint8_t *pUPER = NULL;
    int LenUPER = asn1_uper_encode(&pUPER, asn1_type_SAEMessageFrame, &Msg);
    if (LenUPER < 0)
    {
      // oops
    }
    else
    {
      // Allocate the WSM buffer
      uint8_t *pWSM = calloc(1, sizeof(struct Dot3WSMPHdr) + LenUPER);
      if (pWSM == NULL)
      {
        d_printf(D_TST, NULL, "Unable to allocate Tx WSM buffer for RTCM3 (%m).\n");
      }
      else
      {
        // Transmit the WSM.
        struct Dot3WSMPHdr *pHdr = (struct Dot3WSMPHdr *)pWSM;
        pHdr->Tx.Priority   = Ntrip.Param.WSM_TxPrio;
        pHdr->Tx.ExpiryTime = NTRIP_WSMP_EXPIRY_TIME;
        pHdr->Version       = DOT3_WSMP_VERSION_3;
        pHdr->ChannelNumber = Ntrip.Param.WSM_TxChannel;
        pHdr->DataRate      = Ntrip.Param.WSM_TxDataRate;
        pHdr->TxPower       = Ntrip.Param.WSM_TxPower;
        pHdr->PSID          = htonl(Ntrip.Param.WSM_TxPSID);
        pHdr->Length        = htons(LenUPER);
        pHdr->HdrExtFlags   = 0x07; // Channel | DataRate | TxPwr
        memcpy(pHdr->Data, pUPER, LenUPER);

        int Res = P1609Tx_SendWSM(pHdr, true, P1609_TX_ATTACH_DEFAULT, PCAP_LOG_TX);
        if (Res != 0)
        {
          d_printf(D_WARN, NULL, "P1609Tx_SendWSM() = %d\n", Res);
        }
      }
      free(pWSM);
    }
    free(pUPER);
    asn1_free_value(asn1_type_SAERTCMcorrections, pRTCMData);
  }
  return;
}

/**
 * @brief NtripRx processing thread
 * @param pArg Generic input parameter pointer
 */
static void *NtripRx_Thread(void *pArg)
{
  d_fnstart(D_TST, NULL, "()\n");
  (void)pArg;

  int Res;
  struct timespec Time = {0, 0};
  static char Recv[4096];

  Util_Nap(1234, &Time);
  Ntrip.Rx.State = NTRIP_STATE_INIT;
  Ntrip.Rx.Fd = -1;
  // message count for ETSI implementation

  // Thread loop
  while (Ntrip.Rx.Thread.Id != (pthread_t)(-1))
  {
    uint32_t Delay_ms = 523; // Polling delay of ~0.5s

    d_printf(D_TST, NULL, "State %d\n", Ntrip.Rx.State);
    switch (Ntrip.Rx.State)
    {
      case NTRIP_STATE_INIT:
        if (Ntrip.Rx.Fd < 0)
        {
          d_assert(strlen(Ntrip.Rx.Hostname) > 0);
          d_assert(strlen(Ntrip.Rx.Port) > 0);
          Res = TCP_OpenPort(Ntrip.Rx.Hostname,
                             atoi(Ntrip.Rx.Port));
          d_printf(D_TST, NULL, "TCP_OpenPort(%s, %d) = %d\n",
                   Ntrip.Rx.Hostname, atoi(Ntrip.Rx.Port), Res);
          if (Res >= 0)
          {
            Ntrip.Rx.Fd = Res;
            Ntrip.Rx.State = NTRIP_STATE_OPEN;
          }
        }
        ; // Otherwise keep trying to reconnect
        break;

      case NTRIP_STATE_OPEN:
        {
          d_assert(Ntrip.Rx.Fd >= 0);
          d_assert(strlen(Ntrip.Rx.Hostname) > 0);
          d_assert(strlen(Ntrip.Rx.Port) > 0);
          d_assert(strlen(Ntrip.Rx.Mountpoint) > 0);

          // Send 'GET'
          char Get[1024] = { 0, };
          size_t Base64Len = 0;
          uint8_t *pBase64 = base64_encode((const unsigned char *)Ntrip.Rx.Credentials,
                                           strlen(Ntrip.Rx.Credentials),
                                           &Base64Len);
          d_printf(D_TST, NULL, "base64 '%s'\n", ((pBase64 != NULL) && (Base64Len > 0)) ? (const char *)pBase64 : "");
          snprintf(Get, sizeof(Get)-1,
                   "GET /%s HTTP/1.1\r\n"
                   "User-Agent: NTRIP JCMBsoftPythonClient/0.2\r\n" // "User-Agent: NTRIP RTCM over SAE J2735\r\n"
                   "Authorization: Basic %s\r\n"
                   "Host: %s:%s\r\n"
                   "Ntrip-Version: Ntrip/2.0\r\n"
                   "\r\n",
                    Ntrip.Rx.Mountpoint,
                    ((pBase64 != NULL) && (Base64Len > 0)) ? (const char *)pBase64 : "",
                    Ntrip.Rx.Hostname,
                    Ntrip.Rx.Port);
          free(pBase64);
          d_printf(D_TST, NULL, "Get: '%s'\n", Get);
          Res = send(Ntrip.Rx.Fd, Get, strlen(Get), MSG_DONTWAIT);
          d_printf(D_TST, NULL, "send(%zu) = %d\n", strlen(Get), Res);
          if (Res == (int)strlen(Get))
          {
            Ntrip.Rx.State = NTRIP_STATE_GET;
          }
          else if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
          {
            // The socket is broken. Reconnect
            close(Ntrip.Rx.Fd);
            Ntrip.Rx.Fd = -1;
            Ntrip.Rx.State = NTRIP_STATE_INIT;
          }
          ; // Otherwise keep trying to resend the 'GET'
        }
        break;

      case NTRIP_STATE_GET:
        d_assert(Ntrip.Rx.Fd >= 0);
        // Await 'GET' response & parse
        Res = recv(Ntrip.Rx.Fd, Recv, sizeof(Recv) - 1, MSG_DONTWAIT);
        d_printf(D_TST, NULL, "recv() = %d\n", Res);
        if (Res > 0)
        {
          Recv[Res - 1] = '\0';

          if ((strstr(Recv, "ICY 200 OK") != NULL) ||
              (strstr(Recv, "HTTP/1.0 200 OK") != NULL) ||
              (strstr(Recv, "HTTP/1.1 200 OK") != NULL))
          {
            d_printf(D_TST, NULL, "'%s'\n", Recv);
            Ntrip.Rx.State = NTRIP_STATE_200_OK;
          }
          else if ((strstr(Recv, "SOURCETABLE") != NULL) ||
                   (strstr(Recv, "404 Not Found") != NULL))
          {
            d_printf(D_WARN, NULL, "Mount point does not exist: '%s'\n", Recv);
            Ntrip.Rx.State = NTRIP_STATE_EXIT;
          }
          else if (strstr(Recv, "401 Unauthorized") != NULL)
          {
            d_printf(D_WARN, NULL, "Denied! '%s'\n", Recv);
            Ntrip.Rx.State = NTRIP_STATE_EXIT;
          }
          else
          {
            d_printf(D_WARN, NULL, "Unexpected '%s'\n", Recv);
            Ntrip.Rx.State = NTRIP_STATE_EXIT;
          }
        }
        else if ((Res < 0) &&
                 (errno != EINTR) &&
                 (errno != EAGAIN) &&
                 (errno != EWOULDBLOCK))
        {
          // The socket is broken. Reconnect
          close(Ntrip.Rx.Fd);
          Ntrip.Rx.Fd = -1;
          Ntrip.Rx.State = NTRIP_STATE_INIT;
        }
        ; // Otherwise keep trying to collect the 'GET' response
        break;

      case NTRIP_STATE_200_OK:
        {
          d_assert(Ntrip.Rx.Fd >= 0);
          // Send 'GGA'
          char GGA[128] = { 0, };
          Res = NtripRx_GetGGA(GGA, sizeof(GGA));
          if (Res > 0)
          {
            d_printf(D_TST, NULL, "GGA: '%s'\n", GGA);
            Res = send(Ntrip.Rx.Fd, GGA, strlen(GGA), MSG_DONTWAIT);
            d_printf(D_TST, NULL, "send(%zu) = %d\n", strlen(GGA), Res);
            if (Res == (int)strlen(GGA))
            {
              Ntrip.Rx.State = NTRIP_STATE_STREAM;
              Ntrip.Rx.TimestampGGA = Util_Now();
            }
          }
          else if ((Res < 0) &&
                   (errno != EAGAIN) &&
                   (errno != EWOULDBLOCK))
          {
            // The socket is broken. Reconnect
            close(Ntrip.Rx.Fd);
            Ntrip.Rx.Fd = -1;
            Ntrip.Rx.State = NTRIP_STATE_INIT;
          }
        }
        ; // Otherwise keep trying to resend the 'GGA'
        break;

      case NTRIP_STATE_STREAM:
        d_assert(Ntrip.Rx.Fd >= 0);
        // Await 'GET' response & parse
        memset(Recv, 0, min(3, sizeof(Recv)));
        Res = recv(Ntrip.Rx.Fd, Recv, sizeof(Recv), MSG_DONTWAIT);
        d_printf(D_TST, NULL, "recv() = %d\n", Res);

        if (Res > 0)
        {
          int Index = 0;
          tRTCMMsg RTCM[Ntrip_Cust_App_Ntrip_RTCM_MaxPayloads_MAX];

          d_dump(D_TST, NULL, Recv, Res);
          char *pStart = Recv;

          // Parse HTTP data to get message payload
          // [ASCII HEX length] <CR><LF> [RTCM message] <CR><LF> ...
          while (Res > 0)
          {

            if (Conf2_GetProtocolMode() != default_stack_ProtocolMode_1609)
            {
              // Find leading HTTP data separator (<CR><LF>) and skip it
              while (Res > 1)
              {
                if (((uint8_t)pStart[0] == 0x0d) && ((uint8_t)pStart[1] == 0x0a))
                {
                  // Skip separator
                  {
                    Res -= 2;
                    pStart += 2;
                  }
                  break;
                }
                Res--;
                pStart++;
              }
            }

            // RTCM3 message format:
            //   +--------+--------+-----------+----------------+--------+
            //   | 0xd3   | 000000 | Length    | Message bytes  | Parity |
            //   +--------+--------+-----------+----------------+--------+
            //   | 8bits  | 6bits  | 10bits    | Length * 8bits | 24bits |

            if ((pStart != NULL) && ((uint8_t)pStart[0] == 0xd3))
            {
              size_t MsgLen = 0;

              d_printf(D_TST, NULL, "Found start (%zd,0x%02x)\n", pStart-Recv, (uint8_t)pStart[0]);

              MsgLen = ((uint8_t)pStart[1] << 8) + (uint8_t)pStart[2];

              // Skip RTCM header
              if (Res >= 3)
              {
                pStart += 3;
                Res -= 3;
              }
              else
              {
                MsgLen = 0;
                pStart = NULL;
                Res = 0;
              }

              if (Res > (int)MsgLen)
              {
                if (MsgLen > 0)
                {
                  // RTCM3 payload
                  d_printf(D_INFO, NULL, "RTCM3[%zd], ID=%d\n", MsgLen, (pStart[0] << 4) + ((pStart[1] & 0xF0) >> 4));
                  d_dump(D_TST, NULL, pStart, MsgLen);

                  // Save RTCM start point
                  RTCM[Index].pBuf = pStart;
                  RTCM[Index].Len = MsgLen;
                  Index++;

                  d_printf(D_TST, NULL, "Index(%d)\n", Index);
                  if (Index > Ntrip.Param.RTCM_MaxPayloads)
                  {
                    d_printf(D_ERR, NULL, "Exceeding RTCM_MaxPayloads limit (%d), remaining messages ignored!\n", Ntrip.Param.RTCM_MaxPayloads);
                    break;
                  }

                  // Skip message
                  pStart += MsgLen;
                  Res -= MsgLen;

                  // Skip RTCM checksum/parity
                  if (Res >= 3)
                  {
                    pStart += 3;
                    Res -= 3;
                  }
                  else
                  {
                    pStart = NULL;
                    Res = 0;
                  }
                }
              }
              else
              {
                d_printf(D_INFO, NULL, "RTCM3 length error (%zd)\n", MsgLen);
                MsgLen = 0;
              }
            }

      	    if (Conf2_GetProtocolMode() != default_stack_ProtocolMode_1609)
      	    {
              // Find second HTTP data separator and skip it
              while (Res > 1)
              {
                if (((uint8_t)pStart[0] == 0x0d) && ((uint8_t)pStart[1] == 0x0a))
                {
                  // Skip separator
                  {
                    Res -= 2;
                    pStart += 2;
                  }
                  break;
                }
                Res--;
                pStart++;
              }
      	    }
          }

          if (Index > 0)
          {
            // RTCM3 payload
            if (Ntrip.CN == true)
            {
              NtripRx_SendCN(Index, RTCM);
            }
            else if (Ntrip.EU == true)
            {
              NtripRx_SendEU(Index, RTCM);
            }
            else if (Ntrip.NA == true)
            {
              NtripRx_SendNA(Index, RTCM);
            }
            Delay_ms = 100; // It's coming in! Expect ~8Hz
          }
        }
        else if (Res == 0)
        {
          // The socket has been closed. Reconnect
          close(Ntrip.Rx.Fd);
          Ntrip.Rx.Fd = -1;
          Ntrip.Rx.State = NTRIP_STATE_INIT;
        }
        else if ((Res < 0) &&
                 (errno != EINTR) &&
                 (errno != EAGAIN) &&
                 (errno != EWOULDBLOCK))
        {
          // The socket is broken. Reconnect
          close(Ntrip.Rx.Fd);
          Ntrip.Rx.Fd = -1;
          Ntrip.Rx.State = NTRIP_STATE_INIT;
        }
        // Potentially re-send the GGA to keep the connection alive
        if (Util_Now() > (Ntrip.Rx.TimestampGGA + Ntrip.Param.GGA_Interval))
        {
          Ntrip.Rx.State = NTRIP_STATE_200_OK;
        }
        ; // Otherwise keep waiting to collect the 'RTCM' data
        break;

      default:
        d_printf(D_ERR, NULL, "State %d. Exiting...\n", Ntrip.Rx.State);
        Ntrip.Rx.State = NTRIP_STATE_EXIT;
        break;
    }

    // Potential cleanup
    if (Ntrip.Rx.State == NTRIP_STATE_EXIT)
    {
      if (Ntrip.Rx.Fd >= 0)
      {
        close(Ntrip.Rx.Fd);
      }
      Ntrip.Rx.Fd = -1;
      break;
    }

    // State machine polling interval
    Util_Nap(Delay_ms, &Time);
  }

  // Exit thread
  (void)pthread_exit(NULL);
  d_fnend(D_TST, NULL, "()\n");
  return NULL;
}


/**
 * @brief Initialize the thread processing
 */
static int NtripRx_ThreadInit(void)
{
  int Res;

  d_fnstart(D_TST, NULL, "()\n");

  // Start the thread
  Ntrip.Rx.Thread.Id = (pthread_t)(-1);
  Res = Util_StartThread(&(Ntrip.Rx.Thread.Id),
                         SCHED_OTHER,
                         THREAD_LOW_PRIORITY,
                         NtripRx_Thread,
                         &Ntrip);
  if (Res != 0)
  {
    // Thread creation error
    d_printf(D_ERR, NULL, "Ntrip thread creation failed\n");
    Ntrip.Rx.Thread.Id = (pthread_t)(-1);
    goto Error;
  }

Error:
  d_fnend(D_TST, NULL, "()\n");
  return Res;
}


/**
 * @brief De-initialize the thread processing
 */
static void NtripRx_ThreadExit(void)
{
  d_fnstart(D_TST, NULL, "()\n");

  // If the thread started: Signal & wait for it to exit
  if (Ntrip.Rx.Thread.Id != (pthread_t)(-1))
  {
    pthread_t Id = Ntrip.Rx.Thread.Id;
    // Signal thread to terminate
    Ntrip.Rx.Thread.Id = (pthread_t)(-1);
    // All good. Now wait for it to exit
    pthread_join(Id, NULL);
  }
  Ntrip.Rx.State = NTRIP_STATE_NONE;

  d_fnend(D_TST, NULL, "()\n");
}



/**
 * @brief Initialise the Ntrip application
 * @return Zero for success or a negative error
 */
int NtripRx_Init(void)
{
  int Res;

  d_fnstart(D_TST, NULL, "()\n");

  if (Ntrip.Param.ENABLE == 0)
  {
    Res = 0;
    goto Success;
  }

  Res = NtripRx_ParseURI(Ntrip.Param.URI);
  if (Res)
  {
    goto Error;
  }

  Res = NtripRx_ThreadInit();
  if (Res)
  {
    goto Error;
  }

  Res = 0;
  goto Success;

Error:
  NtripRx_Exit();

Success:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}

/**
 * @brief De-initialize the NtripRx
 */
void NtripRx_Exit(void)
{
  d_fnstart(D_TST, NULL, "()\n");

  if (Ntrip.Active)
  {
    NtripRx_ThreadExit();
  }

  d_fnend(D_TST, NULL, "()\n");
}


#if defined(UNITTEST)

void UNITTEST_Foo(int Bar)
{
  ; // Foo(Bar);
}

#endif

/**
 * @}
 */
