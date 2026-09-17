/*
 * PROTOCOL_FRAMER.H
 * Builds outgoing protocol frames from a payload struct + type.
 */

#ifndef PROTOCOL_FRAMER_H
#define PROTOCOL_FRAMER_H

#include "protocol.h"

/* Builds a complete frame (SYNC+TYPE+LENGTH+PAYLOAD+CRC) into out_buf.
 *
 *   type          : FRAME_TYPE_* value
 *   payload       : pointer to the payload struct to serialize
 *   payload_len   : size of that payload in bytes
 *   out_buf       : caller-provided buffer, must be at least
 *                   PROTOCOL_MAX_FRAME_SIZE bytes
 *   out_buf_size  : size of out_buf, checked against actual frame size
 *                   (defensive -- catches a caller passing too small a
 *                   buffer rather than silently overflowing it)
 *
 * Returns the number of bytes written to out_buf, or 0 on error
 * (payload_len too large, or out_buf too small for the resulting frame).
 */
size_t protocol_build_frame(frame_type_t type,
                             const void *payload, uint8_t payload_len,
                             uint8_t *out_buf, size_t out_buf_size);

#endif /* PROTOCOL_FRAMER_H */
