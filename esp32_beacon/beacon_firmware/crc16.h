/*
 * CRC16.H (ESP32/Arduino port)
 * Identical CRC-16-CCITT algorithm to stm32_firmware/inc/crc16.h --
 * same polynomial (0x1021), same initial value (0xFFFF), bit-by-bit
 * (not table-driven, same flash-footprint tradeoff reasoning as the
 * STM32 version, even though ESP32 has far more flash to spare -- kept
 * identical so the algorithm itself is provably unchanged across
 * platforms, not just "close enough").
 */

#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

#define CRC16_INIT 0xFFFFu

uint16_t crc16_update(uint16_t crc, uint8_t byte);
uint16_t crc16_buffer(const uint8_t *data, size_t length);

#endif /* CRC16_H */
