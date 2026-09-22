#pragma once
#include <cstdint>

// On the host, "flash" is ordinary memory.
#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_byte_near(addr) pgm_read_byte(addr)
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define pgm_read_word_near(addr) pgm_read_word(addr)
