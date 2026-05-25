# Cohda MK6 V2X JSON Logger

**Author:** Vaishnavi Balambeed  
**Platform:** Cohda Wireless MK6 OBU (aarch64)  
**SDK Version:** Cohda MK6 SDK, October 2024  
**Build Date:** May 2026  

---

## Overview

This is a modified version of the Cohda MK6 `example1609` reference application.  
It adds three independent message loggers that run alongside the standard OBE stack:

| Logger | Message Type | Log File | Default UDP Port |
|--------|-------------|----------|-----------------|
| `bsm-log` | SAE J2735 BSM (Basic Safety Message) | `bsm_log.jsonl` | 5008 |
| `spatmap-log` | SAE J2735 SPAT + MAP | `spatmap_log.jsonl` | 5005 |
| `sdsm-log` | SAE J2735 SDSM (Sensor Data Sharing Message) | `sdsm_log.jsonl` | 5007 |

### What makes this different from the stock example

The stock example1609 logs messages field-by-field into a fixed 32KB buffer, dropping
optional fields and truncating large messages.

This version uses the **ASN.1 JER (JSON Encoding Rules)** encoder (`asn1_jer_encode2`)
to serialize the **complete decoded C struct** directly to JSON:

- Every field present in the over-the-air message is included
- OPTIONAL fields absent from the message are omitted automatically
- ENUMERATED values become human-readable strings (`"stop-And-Remain"` not `3`)
- BIT STRING fields become hex strings (`"8000"` = straight maneuver)
- No size cap — `jer_buf` is heap-allocated per message, sent via `sendmsg()` iovec
- Pretty-printed output (2-space indent) for readability

### Example BSM record

```json
{
  "msg_type": "BSM",
  "ts_rx_ms": 1779643281559,
  "message": {
    "coreData": {
      "msgCnt": 9,
      "id": "6ccb119b",
      "lat": 422404053,
      "long": -836757767,
      "elev": 2321,
      "speed": 1,
      "heading": 0,
      "brakes": { "wheelBrakes": "00", "traction": "unavailable" },
      "size": { "width": 203, "length": 532 }
    },
    "partII": [
      {
        "partII-Id": 0,
        "partII-Value": {
          "pathHistory": { "crumbData": [ { "latOffset": -203, "lonOffset": 114 } ] },
          "pathPrediction": { "radiusOfCurve": 32767, "confidence": 200 }
        }
      }
    ]
  }
}
```

---

## Repository Structure

```
src/
├── example1609.c          # Main application — reads obu.cfg, initialises all modules
├── bsm-log/
│   ├── bsm-log.c          # BSM logger: Ext callback + JER encode + file + UDP
│   └── bsm-log.h
├── spatmap-log/
│   ├── spatmap-log.c      # SPAT/MAP logger
│   └── spatmap-log.h
├── sdsm-log/
│   ├── sdsm-log.c         # SDSM logger
│   └── sdsm-log.h
├── obe-rx/                # OBE receive module (stock Cohda)
├── spat-tx/               # SPAT transmit module (stock Cohda)
├── map-tx/                # MAP transmit module (stock Cohda)
├── sdsm-tx/               # SDSM transmit module (stock Cohda)
└── ...                    # Other stock modules
obu.cfg                    # App-level config (enable/disable each module)
obu.conf                   # Stack-level config (UDP ports, channel, security)
rsu.cfg / rsu.conf         # RSU mode config
Makefile                   # Cross-compile + tarball target
```

---

## Prerequisites

1. **Cohda MK6 SDK** — October 2024 release  
   The SDK provides the cross-compiler toolchain, `v2x-lib` headers and libraries.  
   Place this repo at `<sdk_root>/stack/apps/example1609/`.

2. **Cross-compiler:** `aarch64-linux-gnu-gcc` (included with the SDK)

3. **Target hardware:** Cohda Wireless MK6 OBU

---

## Integration into the SDK

### 1. Place source in the SDK tree

```bash
# From the SDK root (e.g. ~/mk6/)
cd stack/apps/

# If starting fresh — clone into the example1609 slot
git clone https://github.com/VaishnaviMichiganTech/cohda-mk6-v2x-json-logger.git example1609
```

Or replace the existing example1609 source files:
```bash
cp -r cohda-mk6-v2x-json-logger/src/*   stack/apps/example1609/src/
cp    cohda-mk6-v2x-json-logger/Makefile stack/apps/example1609/
```

### 2. Build

```bash
cd stack/apps/example1609
make mk6-tarball
```

This produces `example1609-mk6-Exported.tgz` (~15 MB) containing the binary,
all required shared libraries, and configuration files.

### 3. Deploy to MK6 OBU

```bash
# Copy tarball to OBU (replace IP with your OBU's address)
scp example1609-mk6-Exported.tgz user@192.168.222.71:/tmp

# SSH in and extract
ssh user@192.168.222.71
cd /mnt/rw
tar xzf /tmp/example1609-mk6-Exported.tgz
```

---

## Configuration

### Enable loggers in `obu.cfg`

Add to the `Example.APP` section:

```
APP = {
    OBERx      = true;
    SPATMAPLog = true;
    SDSMLog    = true;
    BSMLog     = true;
    # ... other modules
};
```

### Configure UDP forwarding in `obu.conf`

Add these blocks **before** the `%` include lines at the end of `obu.conf`:

```
BSMLog:
{
    UDP_Host = "127.0.0.1";
    UDP_Port = 5008;
};

SDSMLog:
{
    UDP_Host = "127.0.0.1";
    UDP_Port = 5007;
};

SpatMapLog:
{
    UDP_Host = "127.0.0.1";
    UDP_Port = 5005;
};
```

> IPv6 link-local addresses are also supported:
> `UDP_Host = "fe80::4e2f:b578:6c6e:5b09%eth0";`

### Run

```bash
cd /mnt/rw/example1609

# OBU mode (receive BSM/SPAT/MAP/SDSM, log everything)
./rc.example1609 start obu

# RSU mode (transmit SPAT/MAP/SDSM)
./rc.example1609 start rsu

# Stop
./rc.example1609 stop
```

Log files are written to the timestamped log directory shown at startup:
```
Log directory is /mnt/src/log/2026.0524.1742_<serial>_<pid>
```

---

## Design Notes

### Type-safe message dispatch

All three loggers register a single `Ext_CallbackRegister` callback. Every received
WSM fires all three callbacks. Each callback checks `pMsg->pType` before processing:

```c
// Correct — identity check on the type descriptor pointer
if (pMsg->pType != (const uintptr_t *)asn1_type_SAEBasicSafetyMessage) return;

// Wrong (original bug) — pBSM aliases pSAESPAT in the union;
// non-NULL for SPAT/MAP/SDSM → segfault when serializing wrong struct
if (pMsg->pBSM == NULL) return;
```

### UDP scatter-gather (zero copy)

`sendmsg()` with `struct iovec[3]` sends prefix + `jer_buf` + suffix as one
UDP datagram without copying into an intermediate buffer:

```c
struct iovec iov[3];
iov[0] = { prefix,  prefix_len };   // stack — ~60 bytes
iov[1] = { jer_buf, jer_len    };   // heap  — any size
iov[2] = { suffix,  suffix_len };   // static literal
sendmsg(g_udp_fd, &mhdr, 0);
```

### obu.conf block extraction

`obu.conf` uses Cohda-specific syntax (`BSMEnabled = 1` without semicolons,
range annotations) that `libconfig` cannot parse. Each logger extracts only
its own `BSMLog:{...}` block into a temp file before calling `config_read_file`.

---

## License

Source code in `src/bsm-log/`, `src/spatmap-log/`, `src/sdsm-log/` written by
Vaishnavi Balambeed. All other modules are part of the Cohda Wireless MK6
example1609 reference application and are subject to Cohda's SDK license terms.
