# Telemetry/Command Protocol — Design Notes

## Frame format

```
| SYNC1 | SYNC2 | TYPE | LENGTH | PAYLOAD (0-64B) | CRC16 (2B, little-endian) |
|  1B   |  1B   |  1B  |   1B   |    LENGTH B     |            2B             |
```

- **SYNC** = `0xAA 0x55`, fixed. Lets the parser find frame boundaries in a
  continuous byte stream and resynchronize after noise/garbage — without
  it, a single corrupted length byte could desync the parser permanently.
- **TYPE**: `0x01`=Telemetry, `0x02`=Command, `0x03`=Ack/Nack.
- **LENGTH**: payload byte count (max 64), so the parser knows exactly
  how many bytes to expect before the CRC.
- **CRC16**: CRC-16-CCITT (poly `0x1021`), computed over TYPE+LENGTH+PAYLOAD
  only — not SYNC, since SYNC is a fixed marker with no error-detection
  value to protect.

## Why CCSDS-inspired, not full CCSDS

Real CCSDS Space Packet Protocol has a much larger field set (version
number, packet type, secondary header flag, APID, sequence flags, packet
data length as a 16-bit field, and typically a separate secondary header
for time). This project deliberately implements a **simplified, scoped-down**
version — same conceptual structure (sync/framing, typed payloads, length
field, integrity check), but sized appropriately for a from-scratch
learning/portfolio project rather than a full standards-compliance effort.
This is stated explicitly rather than silently passed off as full CCSDS.

## Why the parser is a state machine, not a buffer-parsing function

Real UART reception delivers bytes one at a time via interrupt, with
arbitrary timing gaps — there's no guarantee a whole frame is available
in memory at once. A function written as "assume the complete frame array
is already here" passes host unit tests trivially (you hand it a
pre-assembled array) and then breaks the first time it meets actual
hardware timing. Building and testing the incremental state-machine
version from day one avoids that gap between "passes my tests" and
"works on real UART" — see `tests/test_harness.c` Test 4 specifically,
which feeds a frame in two separate calls to prove state survives
correctly across what would be separate interrupt events.

## Real bug found and fixed during development

The parser's completion handler originally called the general-purpose
`protocol_parser_init()` before returning `PARSE_RESULT_FRAME_OK` —
which resets `ctx->type`/`payload`/`payload_len` along with the state
machine bookkeeping, wiping the very data the caller needed to read
*immediately after* getting that success result back. Fixed by splitting
into two reset functions: a public `protocol_parser_init()` for full
external resets, and an internal `parser_reset_for_next_frame()` that
only resets state-machine bookkeeping, leaving parsed frame data intact
for the caller. Caught by actually compiling and running the test suite
(`test_harness.c` Test 5 initially failed, which led to isolating and
fixing this) rather than assumed correct from code review alone.

## Telemetry payload field choice

`telemetry_payload_t` carries `attitude_error_deg`, `omega[3]`, and
`h_wheel[3]` — not a generic placeholder struct, but the actual output
fields the completed CubeSat ADCS project's control loop produces. This
is a deliberate connective choice: a real flight computer running that
ADCS logic would need to downlink exactly this data, so this protocol is
designed as if it were the actual telemetry interface for that prior work,
not an unrelated exercise.

## CRC width and implementation choice

CRC-16-CCITT (bit-by-bit, not table-driven) — adequate error-detection
strength for frames this short (≤64B payload), computable one byte at a
time (matches the state-machine parser's incremental design), and avoids
the 512-byte flash cost of a lookup table on a resource-constrained MCU.
Documented as a deliberate embedded-systems tradeoff, not an oversight —
see `src/crc16.c` for the note on swapping to table-driven if a future
higher-throughput use case needs it.

## Verification

All logic verified via `tests/test_harness.c`, compiled and run on-host
with plain `gcc -Wall -Wextra` (zero warnings), 16/16 tests passing,
covering: normal round-trip, corrupted-frame rejection, resync after
garbage bytes, frame delivery split across multiple calls (simulating
separate UART interrupts), oversized-length rejection, and correct
recovery/continued operation after a CRC error on a prior frame.

Hardware-in-the-loop UART validation on real STM32 hardware is planned
once a board is acquired — the portable core logic (framer/parser/CRC)
is written with zero HAL dependencies specifically so it drops into
`main_stub.c`'s UART RX interrupt handler unchanged when that happens.
