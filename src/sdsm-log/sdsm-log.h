#pragma once

/**
 * @file sdsm-log.h
 * @brief SDSM receiver with simultaneous file logging and UDP forwarding.
 *
 * Registers an Ext callback for QSMSG_EXT_RX_WSM events. Every received
 * SDSM packet is decoded (pre-decoded by the stack via pMsg->pSAESDSM),
 * written as a JSON Lines entry to a log file, AND forwarded over UDP to
 * a configurable destination — zero additional delay.
 *
 * UDP configuration (add to obu.conf BEFORE the % include lines):
 *
 *   SDSMLog:
 *   {
 *       UDP_Host = "127.0.0.1";   // destination IP
 *       UDP_Port = 5007;           // destination port
 *   };
 *
 * If the keys are absent, defaults are used: 127.0.0.1:5007
 * Set UDP_Port = 0 to disable UDP (file logging only).
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise SDSMLog.
 *        Registers WSM Rx callback, opens log file, reads UDP config,
 *        opens UDP socket.
 *
 * @param pStackConfigFilename  Path to the stack .conf file (obu.conf).
 *                              Pass NULL to use built-in defaults.
 * @return 0 on success, negative on error.
 */
int SDSMLog_Init(const char *pStackConfigFilename);

/**
 * @brief De-initialise SDSMLog.
 *        Deregisters callback, flushes and closes log file,
 *        closes UDP socket.
 */
void SDSMLog_Exit(void);

#ifdef __cplusplus
}
#endif
