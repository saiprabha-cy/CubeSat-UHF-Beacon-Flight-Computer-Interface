/*
 * PROTOCOL_PARSER.H
 * Incremental (byte-at-a-time) frame parser, implemented as an explicit
 * state machine.
 *
 * WHY A STATE MACHINE, NOT "PARSE A COMPLETE BUFFER": real UART reception
 * delivers bytes one at a time via interrupt, with arbitrary gaps between
 * them -- there is no guarantee a whole frame is available at once. A
 * parser written as "assume the full frame array is already here" works
 * perfectly in host unit tests (where you hand it a complete byte array)
 * and then breaks the first time it meets real hardware timing. Building
 * and testing the state-machine version from the start avoids that gap
 * between "passes my tests" and "works on real UART."
 */

#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include "protocol.h"

typedef enum {
    PARSE_STATE_WAIT_SYNC1,
    PARSE_STATE_WAIT_SYNC2,
    PARSE_STATE_READ_TYPE,
    PARSE_STATE_READ_LENGTH,
    PARSE_STATE_READ_PAYLOAD,
    PARSE_STATE_READ_CRC_LOW,
    PARSE_STATE_READ_CRC_HIGH
} parse_state_t;

typedef enum {
    PARSE_RESULT_INCOMPLETE,   /* need more bytes -- normal, keep feeding */
    PARSE_RESULT_FRAME_OK,     /* complete frame, CRC valid -- read ctx->type/payload/payload_len */
    PARSE_RESULT_CRC_ERROR,    /* complete frame, but CRC mismatch -- data corrupted, frame discarded */
    PARSE_RESULT_LENGTH_ERROR  /* LENGTH byte exceeded PROTOCOL_MAX_PAYLOAD -- malformed/corrupt frame */
} parse_result_t;

typedef struct {
    parse_state_t state;
    frame_type_t  type;
    uint8_t       payload[PROTOCOL_MAX_PAYLOAD];
    uint8_t       payload_len;
    uint8_t       payload_idx;      /* bytes of payload received so far */
    uint16_t      running_crc;      /* incremental CRC over TYPE+LENGTH+PAYLOAD */
    uint16_t      received_crc;     /* CRC bytes as received on the wire */
} parser_ctx_t;

/* Must be called once before first use, and again after any
 * PARSE_RESULT_CRC_ERROR / PARSE_RESULT_LENGTH_ERROR to reset the state
 * machine back to hunting for the next SYNC1 -- a corrupted frame must
 * not leave the parser stuck. */
void protocol_parser_init(parser_ctx_t *ctx);

/* Feed one byte into the state machine. Call this once per byte as it
 * arrives (e.g. from a UART RX interrupt in the real firmware, or from a
 * test harness stepping through a byte array). */
parse_result_t protocol_parser_feed_byte(parser_ctx_t *ctx, uint8_t byte);

#endif /* PROTOCOL_PARSER_H */
