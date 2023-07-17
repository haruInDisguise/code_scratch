#include <assert.h>

#include "hashmap.h"

#define NUM_ELEMENTS 32
#define KEY_SIZE 16

static uint32_t string_hash_func(char *key, uint32_t seed) {
    uint32_t hash = 0;

    for (uint32_t i = 0; i < strlen(key); i++) {
        hash += (key[i] * seed + i);
    }

    return hash;
}

#if 0
int main(void) {
    hm_HashMap *map = hm_new(string_hash_func, 3242399, 324239941);

    for(uint32_t index = 0; index < NUM_ELEMENTS; index ++) {
        hm_insert(map, "some value", &(int){42});
    }

    hm_destroy(map);

    return 0;
}

#else
// avoding bitfiels?: https://stackoverflow.com/questions/46021790/are-there-reasons-to-avoid-bit-field-structure-members
int main(int argc, char **argv) {
    struct {
        char buffer[KEY_SIZE];
        uint32_t value;
    } *pairs = calloc(NUM_ELEMENTS, sizeof(*pairs));

    hm_HashMap *map = hm_new(string_hash_func, 3242399, 324239941);
    uint32_t index = 0;

    for (index = 0; index < NUM_ELEMENTS; index++) {
        // NOTE: Look into __attribute__((format(...)))
        snprintf(pairs[index].buffer, NUM_ELEMENTS, "awesome_key_%u", index);
        hm_insert(map, pairs[index].buffer, &pairs[index].value);
        pairs[index].value = index;
    }

    for (index = 0; index < NUM_ELEMENTS; index++) {
        uint32_t *value = hm_get(map, pairs[index].buffer);
        assert(*value == index);
    }

    hm_destroy(map);
    free(pairs);

    return 0;
}
#endif
