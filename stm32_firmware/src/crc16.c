/*
 * CRC16.C
 * CRC-16-CCITT implementation, bit-by-bit (not table-driven).
 *
 * DESIGN NOTE: a table-driven CRC (256-entry lookup table) is faster but
 * costs 512 bytes of flash/ROM -- a real consideration on a resource-
 * constrained MCU. For a protocol handling telemetry frames at a modest
 * rate (not a high-throughput bulk-data link), the bit-by-bit version's
 * speed is entirely adequate, and saving the table's flash footprint is
 * the more defensible embedded-systems tradeoff here. Documented as a
 * deliberate choice, not an oversight -- if this protocol later needs to
 * handle much higher frame rates, switching to a table-driven version is
 * a drop-in replacement (same function signatures).
 */

#include "crc16.h"

uint16_t crc16_update(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)byte << 8;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x8000u) {
            crc = (uint16_t)((crc << 1) ^ 0x1021u);
        } else {
            crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

uint16_t crc16_buffer(const uint8_t *data, size_t length)
{
    uint16_t crc = CRC16_INIT;
    for (size_t i = 0; i < length; i++) {
        crc = crc16_update(crc, data[i]);
    }
    return crc;
}
