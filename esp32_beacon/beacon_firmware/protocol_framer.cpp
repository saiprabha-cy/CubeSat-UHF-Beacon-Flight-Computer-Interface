/*
 * PROTOCOL_FRAMER.CPP (ESP32/Arduino port)
 * Identical logic to stm32_firmware/src/protocol_framer.c -- this is
 * the function the main sketch calls to build each outgoing telemetry
 * frame, and it is byte-for-byte the same algorithm already verified
 * by Stage 1's 16/16 host-run unit tests.
 */

#include "protocol_framer.h"
#include "crc16.h"
#include <string.h>

size_t protocol_build_frame(frame_type_t type,
                             const void *payload, uint8_t payload_len,
                             uint8_t *out_buf, size_t out_buf_size)
{
    if (payload_len > PROTOCOL_MAX_PAYLOAD) {
        return 0;
    }

    size_t frame_size = 2u + 1u + 1u + payload_len + 2u;
    if (out_buf_size < frame_size) {
        return 0;
    }

    size_t idx = 0;
    out_buf[idx++] = PROTOCOL_SYNC1;
    out_buf[idx++] = PROTOCOL_SYNC2;
    out_buf[idx++] = (uint8_t)type;
    out_buf[idx++] = payload_len;

    if (payload_len > 0 && payload != NULL) {
        memcpy(&out_buf[idx], payload, payload_len);
        idx += payload_len;
    }

    uint16_t crc = crc16_buffer(&out_buf[2], idx - 2);
    out_buf[idx++] = (uint8_t)(crc & 0xFFu);
    out_buf[idx++] = (uint8_t)((crc >> 8) & 0xFFu);

    return idx;
}
