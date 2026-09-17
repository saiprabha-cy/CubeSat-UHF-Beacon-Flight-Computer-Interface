/*
 * CRC16.H
 * CRC-16-CCITT (polynomial 0x1021, initial value 0xFFFF), the standard
 * choice for framed serial protocols -- good error-detection strength for
 * short frames (our payload is <=64 bytes), single-byte-at-a-time
 * computation (works naturally with byte-by-byte UART reception), and a
 * width (16 bits) that matches our frame's CRC field exactly.
 */

#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

/* Initial CRC value -- pass this as the starting 'crc' argument for a
 * fresh computation, or the running value when feeding bytes one at a
 * time during incremental (state-machine) parsing. */
#define CRC16_INIT 0xFFFFu

/* Update a running CRC with a single byte. Used both by the
 * whole-buffer helper below AND directly by the incremental parser
 * state machine in protocol_parser.c, which cannot wait for a complete
 * buffer before starting the CRC computation. */
uint16_t crc16_update(uint16_t crc, uint8_t byte);

/* Convenience wrapper: compute the CRC over a complete buffer in one call.
 * Used by protocol_framer.c, which always has the full frame available
 * when building an outgoing telemetry frame. */
uint16_t crc16_buffer(const uint8_t *data, size_t length);

#endif /* CRC16_H */
