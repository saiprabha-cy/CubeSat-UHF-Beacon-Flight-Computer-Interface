/*
 * PROTOCOL_FRAMER.H (ESP32/Arduino port)
 * Identical interface to stm32_firmware/inc/protocol_framer.h.
 */

#ifndef PROTOCOL_FRAMER_H
#define PROTOCOL_FRAMER_H

#include "protocol.h"

size_t protocol_build_frame(frame_type_t type,
                             const void *payload, uint8_t payload_len,
                             uint8_t *out_buf, size_t out_buf_size);

#endif /* PROTOCOL_FRAMER_H */
