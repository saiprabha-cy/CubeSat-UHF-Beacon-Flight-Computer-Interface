/*
 * PROTOCOL.H
 * Frame format definitions for the CubeSat beacon telemetry/command
 * protocol. This is the SHARED SPEC -- the STM32 firmware here and the
 * ESP32 beacon firmware (Stage 6) both implement this exact format, which
 * is what makes "same protocol on two platforms" a provable claim rather
 * than an assertion.
 *
 * Frame layout on the wire (all multi-byte fields little-endian):
 *
 *   | SYNC1 | SYNC2 | TYPE | LENGTH | PAYLOAD (0-64B) | CRC16 (2B) |
 *   |  1B   |  1B   |  1B  |   1B   |    LENGTH B     |     2B     |
 *
 * CRC16 is computed over TYPE + LENGTH + PAYLOAD only (NOT the SYNC
 * bytes -- SYNC is a fixed marker for frame-boundary detection, not data
 * worth protecting; including it would only make the CRC a constant
 * offset, adding no error-detection value).
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define PROTOCOL_SYNC1        0xAAu
#define PROTOCOL_SYNC2        0x55u
#define PROTOCOL_MAX_PAYLOAD  64u

/* Maximum possible full-frame size: 2 (sync) + 1 (type) + 1 (length)
 * + MAX_PAYLOAD + 2 (crc) */
#define PROTOCOL_MAX_FRAME_SIZE (2u + 1u + 1u + PROTOCOL_MAX_PAYLOAD + 2u)

typedef enum {
    FRAME_TYPE_TELEMETRY = 0x01,
    FRAME_TYPE_COMMAND   = 0x02,
    FRAME_TYPE_ACK_NACK  = 0x03
} frame_type_t;

/* --- Telemetry payload -------------------------------------------------
 * Deliberately shaped to carry real output from the completed CubeSat
 * ADCS project (attitude error, omega, wheel momentum) -- this is not a
 * generic placeholder struct, it's the actual telemetry a flight
 * computer running that ADCS control loop would need to downlink.
 *
 * Packed size: 4 + 4 + 4*3 + 4*3 + 2 = 34 bytes, well under the 64-byte
 * payload limit. */
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp_ms;
    float    attitude_error_deg;
    float    omega[3];       /* rad/s, body frame */
    float    h_wheel[3];     /* N*m*s, reaction wheel momentum */
    uint16_t seq_counter;    /* increments each telemetry frame, lets the
                                 ground station detect dropped frames */
} telemetry_payload_t;

/* --- Command payload ----------------------------------------------------
 * Kept intentionally simple for Stage 1 -- extend params[] usage per
 * command_id as real commands are added. */
typedef struct {
    uint8_t command_id;
    uint8_t params[8];
} command_payload_t;

#define CMD_SET_MODE          0x01u
#define CMD_REQUEST_TELEMETRY 0x02u
#pragma pack(pop)

/* --- Ack/Nack payload ---------------------------------------------------
 * Minimal: which frame (by seq or command_id) is being acknowledged, and
 * whether it succeeded. */
#pragma pack(push, 1)
typedef struct {
    uint8_t  ack_ok;          /* 1 = ACK, 0 = NACK */
    uint8_t  reason_code;     /* meaningful only when ack_ok == 0 */
} ack_nack_payload_t;
#pragma pack(pop)

#endif /* PROTOCOL_H */
