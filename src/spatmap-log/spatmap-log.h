#pragma once

/**
 * @file spatmap-log.h
 *
 * Logs SPAT/MAP intersection data to a JSONL file and forwards each
 * JSON message over UDP simultaneously.
 *
 * UDP destination (host + port) is read from the stack config file
 * (.conf) at init time — no recompilation needed to change the target.
 *
 * Config keys (add to obu.local.conf or obu.conf):
 *   SpatMapLog.UDP_Host = 127.0.0.1   # destination IP (no semicolon)
 *   SpatMapLog.UDP_Port = 5005;        # destination port
 *
 * If the keys are absent, defaults are used:
 *   Host: 127.0.0.1
 *   Port: 5005
 *
 * If UDP_Port = 0, UDP forwarding is disabled (file logging only).
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise SPATMAPLog — open log file, read UDP config, open socket.
 *
 * @param pStackConfigFilename  Path to the stack .conf file (e.g. obu.conf).
 *                              Pass NULL to use built-in defaults.
 * @return 0 on success, negative on error.
 */
int SpatMapLog_Init(const char *pStackConfigFilename);

/**
 * @brief De-initialise SPATMAPLog — flush/close file, close UDP socket.
 */
void SpatMapLog_Exit(void);

#ifdef __cplusplus
}
#endif
