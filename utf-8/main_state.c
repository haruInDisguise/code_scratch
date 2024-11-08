#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Unicode
 * - Bytes are collected into a 32bit 'value' and compared to a bitmask:
 *      - The mask 'mask' validates the starting sequence byte. The result must match 'pattern'
 *      - The mask 'expect' ensures that the codepoint value is within the min/max range
 *
 * surrogate    : U+D800 - U+DFFF
 * 1 byte
 * range   : U+0000 - U+007F
 * min     : .0000000 (0x00)
 * max     : .1111111 (0x7f)
 * mask    : 1....... (0x80)
 * pattern : 0....... (0x00)
 *
 * 2 bytes
 * invalid      : 0xC0, 0xC1
 * range        : U+0080 - U+07FF
 * layout       : 110xxxyy 10yyzzzz
 * min          : ...00010 ..000000
 * max          : ...11111 ..111111
 * code_mask    : ...1111. ........ (0x1E)
 * pattern      : 110..... 10...... (0xC0, 0x80)
 * mask         : 111..... 11...... (0xE0, 0xC0)
 *
 * 3 bytes
 * range        : U+0800 - U+FFFF
 * layout       : 1110wwww 10xxxxyy 10yyzzzz
 * min          : ....0000 ..100000 ..000000
 * max          : ....1111 ..111111 ..111111
 * code_mask    : ....1111 ..100000 ........ (0x0F, 0x20, 0x00)
 * surr_mask    : 11101101 10100000
 * pattern      : 1110.... 10...... 10...... (0xE0, 0x80, 0x80)
 * mask         : 1111.... 11...... 11...... (0xF0, 0xC0, 0xC0)
 *
 * 4 bytes
 * range        : U+01000000 - U+10FFFF
 * layout       : 11110uvv 10vvwwww 10xxxxyy 10yyzzzz
 * min          : ........ ...10000 ..000000 ..000000
 * max          : .....100 ..001111 ..111111 ..111111
 * code_mask    : .....111 ..11.... ........ ........ (0x07, 0x30)
 * pattern      : 11110... 10...... 10...... 10...... (0xF0, 0x80, 0x80, 0x80)
 * mask         : 11111... 11...... 11...... 11...... (0xF8, 0xC0, 0xC0, 0xC0)
 * */

uint8_t is_valid(uint8_t *buffer, uint32_t buffer_size) {
    static const uint32_t mask_table[4] = {
        0x80000000UL,
        0xE0C00000UL,
        0xF0C0C000UL,
        0xF8C0C0C0UL,
    };

    /* Check basic layout */
    static const uint32_t pattern_table[4] = {
        0x00000000UL,
        0xC0800000UL,
        0xE0808000UL,
        0xF0808080UL,
    };

    /* Check for overlong encoding */
    static const uint32_t code_table[4] = {
        0x00000000UL,
        0x1E000000UL,
        0x0F200000UL,
        0x07300000UL,
    };

    static const uint32_t surrogat_table[] = {
        0x00,
        0xED,
        0xEDA0,
        0xEDA00000,
    };

    /* The top for bits of the starting byte encode the type of the starting byte and therefore the
     * length of the sequence */
    static const uint8_t length_table[16] = {
        1, 1, 1, 1, 1, 1, 1, 1, /* 0... */
        1, 1, 1, 1,             /* 10.. invalid */
        2, 2,                   /* 110. */
        3,                      /* 1110 */
        4,                      /* 1111 or invalid? */
    };

    enum {
        SEQ_ONE,
        SEQ_TWO,
        SEQ_THREE,
        SEQ_FOUR,
    };

    uint32_t value = 0;
    /* The 4 most significant bytes encode the length of the sequence */
    uint8_t length = length_table[buffer[0] >> 4];

    switch (length < buffer_size ? length : buffer_size) {
    case 4:
        value |= ((uint32_t)buffer[3] << 0);
    case 3:
        value |= ((uint32_t)buffer[2] << 8);
    case 2:
        value |= ((uint32_t)buffer[1] << 16);
    case 1:
        value |= ((uint32_t)buffer[0] << 24);
        break;
    default:
        return 0;
    }

    /* Surrogates are UTF-16 codepoints that do not belong in UTF-8, but some implementations don't
     * care */
    /* They start with 0xED, followed by at least 0xA0 */
    uint32_t has_surrogate = (value >> 16) >= 0xEDA0;
    uint32_t masked = value & mask_table[length - 1];
    uint32_t code = value & code_table[length - 1];

    if (masked == pattern_table[length - 1] && (value & code) && !has_surrogate) {
        return length;
    }

    return 0;
}

bool test_string(char *string, uint32_t string_length) {
    bool status = false;

    for (uint32_t i = 0; i < string_length; i++) {
        uint8_t length = 0;
        if (!is_valid((uint8_t *)string + i, string_length)) {
            return false;
        }

        i += length - 1;
    }

    return status;
}

#define test(expected, ...)                                                               \
    do {                                                                                  \
        char buffer[] = { __VA_ARGS__ };                                                  \
        uint8_t length = is_valid((uint8_t *)buffer, sizeof(buffer) / sizeof(buffer[0])); \
        assert(expected == length);                                                       \
    } while (0)

int main(void) {
    test(0, 0xED, 0xA0, 0x80); // surrogate
    test(0, 0xF0, 0x80);
    test(2, 0xC3, 0xBE);
    test(3, 0xE0, 0xBF, 0xBB);

    test(0, 0xFD); // invalid header
    test(0, 0xFB); // "

    test(0, 0xC3);       // missing byte
    test(0, 0xC3, 0xFF); // invalid 2nd byte
    test(0, 0xC1, 0xBF); // overlong

    test(0, 0xE0);             // missing bytes
    test(0, 0xE0, 0xBF);       // missing byte
    test(0, 0xE0, 0x7F, 0xBF); // invalid 2nd byte
    test(0, 0xE0, 0xBF, 0x7F); // invalid 3rd byte
    test(0, 0xE0, 0x9F, 0xBF); // overlong

    test(0, 0xF3);                   // missing bytes
    test(0, 0xF3, 0xBF);             // "
    test(0, 0xF3, 0xBF, 0xBF);       // "
    test(0, 0xF3, 0x7F, 0xBF, 0xBF); // invalid 2nd byte
    test(0, 0xF3, 0xBF, 0x7F, 0xBF); // "       3rd
    test(0, 0xF3, 0xBF, 0xBF, 0x7F); // "       4th
    test(0, 0xF0, 0x80, 0x81, 0xBF); // overlong
    test(0, 0xF4, 0x90, 0x80, 0x80); // out of range
    test(0, 0xF7, 0xBF, 0xBF, 0xBF); // "

    return 0;
}
