/*
 * PROTOCOL.H (ESP32/Arduino port)
 *
 * IDENTICAL frame format to stm32_firmware/inc/protocol.h -- this is
 * what makes "same protocol, two platforms" a provable claim. Only
 * change from the STM32 version: Arduino's core already provides
 * <stdint.h> types via its own headers, and we don't need the
 * STM32-specific comments about HAL-free portability since this file
 * was already portable C to begin with.
 *
 * Frame layout on the wire (all multi-byte fields little-endian):
 *   | SYNC1 | SYNC2 | TYPE | LENGTH | PAYLOAD (0-64B) | CRC16 (2B) |
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_SYNC1        0xAAu
#define PROTOCOL_SYNC2        0x55u
#define PROTOCOL_MAX_PAYLOAD  64u
#define PROTOCOL_MAX_FRAME_SIZE (2u + 1u + 1u + PROTOCOL_MAX_PAYLOAD + 2u)

typedef enum {
    FRAME_TYPE_TELEMETRY = 0x01,
    FRAME_TYPE_COMMAND   = 0x02,
    FRAME_TYPE_ACK_NACK  = 0x03
} frame_type_t;

/* Telemetry payload -- IDENTICAL layout to stm32_firmware/inc/protocol.h,
 * byte-for-byte, so a frame built here and one built on real STM32
 * firmware are indistinguishable on the wire. Carries the same ADCS
 * project fields as a deliberate connective choice (see Stage 1 notes). */
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp_ms;
    float    attitude_error_deg;
    float    omega[3];
    float    h_wheel[3];
    uint16_t seq_counter;
} telemetry_payload_t;

typedef struct {
    uint8_t command_id;
    uint8_t params[8];
} command_payload_t;

#define CMD_SET_MODE          0x01u
#define CMD_REQUEST_TELEMETRY 0x02u
#pragma pack(pop)

#endif /* PROTOCOL_H */
