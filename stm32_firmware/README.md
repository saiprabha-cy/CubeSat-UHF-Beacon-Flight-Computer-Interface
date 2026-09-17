# Stage 1: Telemetry/Command Protocol Firmware

Defines and validates the frame format used across this entire capstone —
the STM32 flight-computer interface board (Stage 2) is designed around
this protocol's UART requirements, and the ESP32 hardware beacon (Stage 6)
implements this exact same format, making "same protocol, two platforms"
a provable claim rather than an assertion.

## Status

**Core protocol logic complete and verified.** 16/16 unit tests passing
on-host (`gcc -Wall -Wextra`, zero warnings). Hardware-in-the-loop UART
validation on real STM32 hardware pending board acquisition — the
portable logic is written with zero HAL dependencies specifically so it
drops into real firmware unchanged when that happens.

## What's here

- `inc/protocol.h` — frame and payload struct definitions (the shared spec)
- `src/crc16.c` / `inc/crc16.h` — CRC-16-CCITT implementation
- `src/protocol_framer.c` — builds outgoing frames
- `src/protocol_parser.c` — incremental state-machine parser (handles
  partial frames, corruption, resync — not a naive whole-buffer parser)
- `tests/test_harness.c` — host-compiled unit tests, 16/16 passing
- `docs/protocol_design_notes.md` — full design rationale, including a
  real bug found and fixed during development (state-reset ordering bug
  that silently destroyed successfully-parsed frame data)

## Running the tests yourself

```bash
cd tests
gcc -Wall -Wextra -I../inc -o test_harness.exe test_harness.c \
    ../src/protocol_framer.c ../src/protocol_parser.c ../src/crc16.c
./test_harness.exe
```

## Architecture note

`protocol_framer.c`, `protocol_parser.c`, and `crc16.c` contain **zero
STM32 HAL calls** — pure, portable C. Only `main_stub.c` (STM32CubeIDE
project glue) would touch hardware-specific UART functions. This is what
makes on-host testing meaningful (not just possible) and is the same
reason this exact code becomes the reference the ESP32 firmware in
Stage 6 reimplements.
