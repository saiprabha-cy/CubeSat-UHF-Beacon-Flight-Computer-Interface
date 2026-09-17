/*
 * PROTOCOL_FRAMER.C
 * See protocol_framer.h for interface documentation.
 */

#include "protocol_framer.h"
#include "crc16.h"
#include <string.h>

size_t protocol_build_frame(frame_type_t type,
                             const void *payload, uint8_t payload_len,
                             uint8_t *out_buf, size_t out_buf_size)
{
    if (payload_len > PROTOCOL_MAX_PAYLOAD) {
        return 0;   /* reject rather than silently truncate -- a caller
                       passing an oversized payload has a bug worth
                       surfacing, not hiding */
    }

    size_t frame_size = 2u /* sync */ + 1u /* type */ + 1u /* length */
                         + payload_len + 2u /* crc */;
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

    /* CRC covers TYPE+LENGTH+PAYLOAD -- i.e. everything written so far
     * EXCEPT the two SYNC bytes at the start. */
    uint16_t crc = crc16_buffer(&out_buf[2], idx - 2);
    out_buf[idx++] = (uint8_t)(crc & 0xFFu);         /* CRC low byte first */
    out_buf[idx++] = (uint8_t)((crc >> 8) & 0xFFu);  /* CRC high byte */

    return idx;
}
