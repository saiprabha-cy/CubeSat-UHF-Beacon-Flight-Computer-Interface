/*
 * CRC16.CPP (ESP32/Arduino port)
 * Byte-identical algorithm to stm32_firmware/src/crc16.c -- verify by
 * diffing the two files; only the .c -> .cpp extension differs, since
 * Arduino's build system compiles .cpp by default and this code is
 * already valid in both C and C++ (no C++-specific syntax used).
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
