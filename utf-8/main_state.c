#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* I could probably adoppt the byte map from wikipedia as a stats lookup table */
/* https://en.wikipedia.org/wiki/UTF-8
 * I just want to validate a given buffer/string and not  extract its codepoints.
 * Meaning I dont have to worry about codepoints in the tail bytes
 * */

/* Unicode
 * 1 byte
 * range   : U+0000 - U+007F
 * mask    : 0b10000000 (0x80)
 * pattern : 0b10000000 (0x80)
 *
 * 2 bytes
 * layout  : byte - utf_tail
 * range   : U+0080 - U+07FF
 * pattern : 0b11000000 (0xC0)
 * mask    : 0b11100000 (0xE0)
 *
 * 3 bytes
 * layout  : byte + utf_tail + utf_tail
 * range   : U+0800 - U+FFFF
 * pattern : 0b11100000 (0xE0)
 * mask    : 0b11110000 (0xF0)
 *
 * 4 bytes
 * layout  : byte + utf_tail + utf_tail + utf_tail
 * range   : U+01000000 - U+10FFFF
 * pattern : 0b11110000 (0xF0)
 * mask    : 0b11111000 (0xF8)
 * */

const uint32_t mask[4] = {
    0x80000000UL,
    0xE0C00000UL,
    0xF0C0C000UL,
    0xF8C0C0C0UL,
};

const uint32_t pattern[4] = {
    0x00000000UL,
    0xC0808080UL,
    0xE0808080UL,
    0xF0808080UL,
};

enum {
    SEQ_ONE,
    SEQ_TWO,
    SEQ_THREE,
    SEQ_FOUR,
};

bool is_valid(uint8_t *buffer, uint32_t buffer_size) {
    uint32_t value = 0x00;
    uint32_t state = SEQ_ONE;

    for (uint32_t i = 0; i < buffer_size; i++) {
        value = value | ((uint32_t)(buffer[i] & 0xff) << (8 * (3 - state)));
        uint32_t tmp = (value & mask[state]);

        printf("buffer=%x value=%x state=%x tmp=%x pattern=%x\n", buffer[i], value, state, tmp, pattern[state]);

        if (tmp != pattern[state]) {
            if(i + 1 >= buffer_size || state == SEQ_FOUR)
                return false;

            state += 1;
            state %= 4;
            continue;
        }

        state = 0;
        value = 0;
    }

    return true;
}

int main(void) {
    uint8_t bytes[] = {197, 0};
    bool status = is_valid(bytes, sizeof(bytes));
    printf("status=%s\n", status ? "true" : "false");

    return 0;
}
