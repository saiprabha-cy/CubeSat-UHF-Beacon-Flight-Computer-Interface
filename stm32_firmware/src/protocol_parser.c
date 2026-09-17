/*
 * PROTOCOL_PARSER.C
 * See protocol_parser.h for design rationale.
 */

#include "protocol_parser.h"
#include "crc16.h"

/* Internal helper: resets ONLY the state-machine bookkeeping (state,
 * payload_idx, running_crc, received_crc) so the parser is ready for the
 * next frame -- deliberately does NOT touch ctx->type/payload/payload_len,
 * unlike the public protocol_parser_init(). This distinction matters: it's
 * called right before returning PARSE_RESULT_FRAME_OK, and if it wiped the
 * parsed fields too, the caller would have nothing valid left to read.
 * ctx->type/payload/payload_len remain valid until the next byte is fed
 * (which starts overwriting payload[] again) -- so the caller must read
 * them immediately after getting FRAME_OK back, before feeding more bytes. */
static void parser_reset_for_next_frame(parser_ctx_t *ctx)
{
    ctx->state = PARSE_STATE_WAIT_SYNC1;
    ctx->payload_idx = 0;
    ctx->running_crc = CRC16_INIT;
    ctx->received_crc = 0;
}

void protocol_parser_init(parser_ctx_t *ctx)
{
    ctx->state = PARSE_STATE_WAIT_SYNC1;
    ctx->payload_len = 0;
    ctx->payload_idx = 0;
    ctx->running_crc = CRC16_INIT;
    ctx->received_crc = 0;
}

parse_result_t protocol_parser_feed_byte(parser_ctx_t *ctx, uint8_t byte)
{
    switch (ctx->state) {

    case PARSE_STATE_WAIT_SYNC1:
        if (byte == PROTOCOL_SYNC1) {
            ctx->state = PARSE_STATE_WAIT_SYNC2;
        }
        /* else: stay here, discard byte -- this is how the parser
         * resynchronizes after noise/garbage on the line: it just keeps
         * scanning for SYNC1 rather than getting stuck. */
        return PARSE_RESULT_INCOMPLETE;

    case PARSE_STATE_WAIT_SYNC2:
        if (byte == PROTOCOL_SYNC2) {
            ctx->state = PARSE_STATE_READ_TYPE;
            ctx->running_crc = CRC16_INIT;   /* CRC starts accumulating
                                                 from TYPE onward */
        } else if (byte == PROTOCOL_SYNC1) {
            /* stay in WAIT_SYNC2 -- handles the edge case of SYNC1
             * appearing twice in a row (e.g. 0xAA 0xAA 0x55...), so a
             * single stray 0xAA byte doesn't desync the whole frame */
        } else {
            ctx->state = PARSE_STATE_WAIT_SYNC1;
        }
        return PARSE_RESULT_INCOMPLETE;

    case PARSE_STATE_READ_TYPE:
        ctx->type = (frame_type_t)byte;
        ctx->running_crc = crc16_update(ctx->running_crc, byte);
        ctx->state = PARSE_STATE_READ_LENGTH;
        return PARSE_RESULT_INCOMPLETE;

    case PARSE_STATE_READ_LENGTH:
        ctx->payload_len = byte;
        ctx->payload_idx = 0;
        ctx->running_crc = crc16_update(ctx->running_crc, byte);
        if (byte > PROTOCOL_MAX_PAYLOAD) {
            /* malformed/corrupted LENGTH -- report the error with
             * payload_len still readable (so a caller logging the
             * error can see what bogus length was received), then
             * reset just the state machine for the next frame */
            parser_reset_for_next_frame(ctx);
            return PARSE_RESULT_LENGTH_ERROR;
        }
        ctx->state = (byte == 0) ? PARSE_STATE_READ_CRC_LOW
                                  : PARSE_STATE_READ_PAYLOAD;
        return PARSE_RESULT_INCOMPLETE;

    case PARSE_STATE_READ_PAYLOAD:
        ctx->payload[ctx->payload_idx++] = byte;
        ctx->running_crc = crc16_update(ctx->running_crc, byte);
        if (ctx->payload_idx >= ctx->payload_len) {
            ctx->state = PARSE_STATE_READ_CRC_LOW;
        }
        return PARSE_RESULT_INCOMPLETE;

    case PARSE_STATE_READ_CRC_LOW:
        ctx->received_crc = byte;   /* low byte first, matches framer's
                                        write order */
        ctx->state = PARSE_STATE_READ_CRC_HIGH;
        return PARSE_RESULT_INCOMPLETE;

    case PARSE_STATE_READ_CRC_HIGH:
        ctx->received_crc |= ((uint16_t)byte << 8);
        {
            parse_result_t result = (ctx->received_crc == ctx->running_crc)
                                     ? PARSE_RESULT_FRAME_OK
                                     : PARSE_RESULT_CRC_ERROR;
            /* Reset only the state-machine bookkeeping, NOT
             * type/payload/payload_len -- on PARSE_RESULT_FRAME_OK the
             * caller reads those fields from ctx immediately after this
             * call returns (see test_harness.c for the correct usage
             * pattern). Using the destructive protocol_parser_init()
             * here was the bug caught and fixed during this project's
             * own development -- worth keeping this note as a record of
             * that, not just silently fixing it. */
            parser_reset_for_next_frame(ctx);
            return result;
        }

    default:
        /* Unreachable in correct operation, but a state machine with no
         * default case is a real, common bug -- fail safe by resetting
         * rather than leaving ctx in an undefined state. */
        parser_reset_for_next_frame(ctx);
        return PARSE_RESULT_INCOMPLETE;
    }
}
