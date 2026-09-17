/*
 * TEST_HARNESS.C
 * Host-compiled unit tests for the protocol framer/parser -- no STM32
 * hardware, no CubeIDE simulator, just plain gcc. This is possible
 * specifically because protocol_framer.c/protocol_parser.c/crc16.c
 * contain zero HAL calls (see stm32_firmware/README.md).
 *
 * Build:  gcc -Wall -Wextra -I../inc -o test_harness.exe test_harness.c \
 *              ../src/protocol_framer.c ../src/protocol_parser.c ../src/crc16.c
 * Run:    ./test_harness.exe
 */

#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "protocol_framer.h"
#include "protocol_parser.h"

static int g_pass = 0, g_total = 0;

#define CHECK(cond, label) do { \
    g_total++; \
    if (cond) { g_pass++; printf("[PASS] %s\n", label); } \
    else      { printf("[FAIL] %s\n", label); } \
} while (0)

/* Feeds bytes into a parser until the first terminal (non-INCOMPLETE)
 * result, then stops and returns it -- matches how a real caller would
 * use this API (react immediately to a completed/errored frame, not
 * blindly keep feeding more bytes into an already-terminal state).
 * Returns PARSE_RESULT_INCOMPLETE if the whole buffer is consumed
 * without reaching a terminal result (a genuinely incomplete frame). */
static parse_result_t feed_all(parser_ctx_t *ctx, const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        parse_result_t r = protocol_parser_feed_byte(ctx, buf[i]);
        if (r != PARSE_RESULT_INCOMPLETE) {
            return r;
        }
    }
    return PARSE_RESULT_INCOMPLETE;
}

/* --- Test 1: round-trip a telemetry frame ------------------------------- */
static void test_telemetry_roundtrip(void)
{
    telemetry_payload_t tx = {
        .timestamp_ms = 123456,
        .attitude_error_deg = 0.491f,
        .omega = {1e-5f, -2e-5f, 3e-5f},
        .h_wheel = {3.046e-4f, -6.814e-4f, 7.529e-4f},
        .seq_counter = 42
    };

    uint8_t buf[PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_len = protocol_build_frame(FRAME_TYPE_TELEMETRY,
        &tx, sizeof(tx), buf, sizeof(buf));
    CHECK(frame_len > 0, "Test 1a: frame built successfully");

    parser_ctx_t ctx;
    protocol_parser_init(&ctx);
    parse_result_t result = feed_all(&ctx, buf, frame_len);
    CHECK(result == PARSE_RESULT_FRAME_OK, "Test 1b: parsed as FRAME_OK");
    CHECK(ctx.type == FRAME_TYPE_TELEMETRY, "Test 1c: correct frame type");
    CHECK(ctx.payload_len == sizeof(tx), "Test 1d: correct payload length");

    telemetry_payload_t rx;
    memcpy(&rx, ctx.payload, sizeof(rx));
    CHECK(rx.timestamp_ms == tx.timestamp_ms, "Test 1e: timestamp round-trips");
    CHECK(rx.seq_counter == tx.seq_counter, "Test 1f: seq_counter round-trips");
    CHECK(rx.attitude_error_deg == tx.attitude_error_deg, "Test 1g: attitude_error_deg round-trips");
}

/* --- Test 2: corrupted frame is rejected, not silently accepted -------- */
static void test_corrupted_frame_rejected(void)
{
    command_payload_t cmd = { .command_id = CMD_REQUEST_TELEMETRY, .params = {0} };
    uint8_t buf[PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_len = protocol_build_frame(FRAME_TYPE_COMMAND, &cmd, sizeof(cmd), buf, sizeof(buf));

    buf[frame_len - 3] ^= 0xFFu;   /* flip bits in the last payload byte */

    parser_ctx_t ctx;
    protocol_parser_init(&ctx);
    parse_result_t result = feed_all(&ctx, buf, frame_len);
    CHECK(result == PARSE_RESULT_CRC_ERROR, "Test 2: corrupted frame correctly rejected (CRC_ERROR)");
}

/* --- Test 3: parser resynchronizes after garbage bytes ------------------ */
static void test_resync_after_garbage(void)
{
    command_payload_t cmd = { .command_id = CMD_SET_MODE, .params = {1,2,3,0,0,0,0,0} };
    uint8_t frame[PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_len = protocol_build_frame(FRAME_TYPE_COMMAND, &cmd, sizeof(cmd), frame, sizeof(frame));

    /* Prepend garbage bytes, including a stray SYNC1 byte on its own,
     * before the real frame -- this is what a noisy UART line looks
     * like in practice. */
    uint8_t stream[PROTOCOL_MAX_FRAME_SIZE + 10];
    uint8_t garbage[] = {0x00, 0x12, PROTOCOL_SYNC1, 0x77, 0x88};
    memcpy(stream, garbage, sizeof(garbage));
    memcpy(stream + sizeof(garbage), frame, frame_len);

    parser_ctx_t ctx;
    protocol_parser_init(&ctx);
    parse_result_t result = feed_all(&ctx, stream, sizeof(garbage) + frame_len);
    CHECK(result == PARSE_RESULT_FRAME_OK, "Test 3a: parser resynced past leading garbage");
    CHECK(ctx.type == FRAME_TYPE_COMMAND, "Test 3b: correct type after resync");
}

/* --- Test 4: partial frame delivered in two separate chunks ------------- */
static void test_partial_frame_across_calls(void)
{
    telemetry_payload_t tx = {
        .timestamp_ms = 999, .attitude_error_deg = 1.0f,
        .omega = {0,0,0}, .h_wheel = {0,0,0}, .seq_counter = 7
    };
    uint8_t buf[PROTOCOL_MAX_FRAME_SIZE];
    size_t frame_len = protocol_build_frame(FRAME_TYPE_TELEMETRY, &tx, sizeof(tx), buf, sizeof(buf));

    parser_ctx_t ctx;
    protocol_parser_init(&ctx);

    /* Feed first half, one byte at a time -- must report INCOMPLETE
     * throughout, never a premature FRAME_OK or error. */
    size_t half = frame_len / 2;
    int ok = 1;
    for (size_t i = 0; i < half; i++) {
        if (protocol_parser_feed_byte(&ctx, buf[i]) != PARSE_RESULT_INCOMPLETE) {
            ok = 0;
        }
    }
    CHECK(ok, "Test 4a: first half of frame reports INCOMPLETE throughout");

    /* Feed the rest -- should now complete correctly, proving state
     * survives correctly across separate calls (simulating separate
     * UART interrupt events, not one bulk feed). */
    parse_result_t result = PARSE_RESULT_INCOMPLETE;
    for (size_t i = half; i < frame_len; i++) {
        result = protocol_parser_feed_byte(&ctx, buf[i]);
    }
    CHECK(result == PARSE_RESULT_FRAME_OK, "Test 4b: second half completes the frame correctly");
}

/* --- Test 5: oversized LENGTH byte is rejected -------------------------- */
static void test_oversized_length_rejected(void)
{
    uint8_t stream[8] = {
        PROTOCOL_SYNC1, PROTOCOL_SYNC2,
        FRAME_TYPE_COMMAND,
        0xFFu,   /* bogus LENGTH, far exceeds PROTOCOL_MAX_PAYLOAD */
        0,0,0,0
    };
    parser_ctx_t ctx;
    protocol_parser_init(&ctx);
    parse_result_t result = feed_all(&ctx, stream, sizeof(stream));
    CHECK(result == PARSE_RESULT_LENGTH_ERROR, "Test 5: oversized LENGTH correctly rejected");
}

/* --- Test 6: parser recovers and correctly parses the NEXT frame after
 * a CRC error on the previous one (proves the reset-for-next-frame fix
 * actually works end-to-end, not just in isolation) -------------------- */
static void test_recovery_after_crc_error(void)
{
    command_payload_t cmd = { .command_id = CMD_SET_MODE, .params = {9,9,9,0,0,0,0,0} };
    uint8_t bad_frame[PROTOCOL_MAX_FRAME_SIZE];
    size_t bad_len = protocol_build_frame(FRAME_TYPE_COMMAND, &cmd, sizeof(cmd), bad_frame, sizeof(bad_frame));
    bad_frame[bad_len - 3] ^= 0xFFu;   /* corrupt it */

    telemetry_payload_t tx = {
        .timestamp_ms = 5, .attitude_error_deg = 2.0f,
        .omega = {0,0,0}, .h_wheel = {0,0,0}, .seq_counter = 1
    };
    uint8_t good_frame[PROTOCOL_MAX_FRAME_SIZE];
    size_t good_len = protocol_build_frame(FRAME_TYPE_TELEMETRY, &tx, sizeof(tx), good_frame, sizeof(good_frame));

    parser_ctx_t ctx;
    protocol_parser_init(&ctx);
    parse_result_t r1 = feed_all(&ctx, bad_frame, bad_len);
    CHECK(r1 == PARSE_RESULT_CRC_ERROR, "Test 6a: first (corrupted) frame reports CRC_ERROR");

    parse_result_t r2 = feed_all(&ctx, good_frame, good_len);
    CHECK(r2 == PARSE_RESULT_FRAME_OK, "Test 6b: parser recovered, second frame parses correctly");
    CHECK(ctx.type == FRAME_TYPE_TELEMETRY, "Test 6c: correct type for the recovered frame");
}

int main(void)
{
    test_telemetry_roundtrip();
    test_corrupted_frame_rejected();
    test_resync_after_garbage();
    test_partial_frame_across_calls();
    test_oversized_length_rejected();
    test_recovery_after_crc_error();

    printf("\n%d / %d tests passed.\n", g_pass, g_total);
    return (g_pass == g_total) ? 0 : 1;
}
