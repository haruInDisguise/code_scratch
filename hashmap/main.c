#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include "hashmap.h"

#define NUM_ELEMENTS 47
#define KEY_SIZE 16

static struct {
    uint32_t total_pairs;
    uint32_t input_length;

    struct KeyValuePair {
        char *key;
        bool was_deleted;
        uint32_t key_size;
        uint32_t value;
    } *key_value_pairs;
    void *input_buffer;
} state;

// Building with address sanitizer extends the runtime by a lot,
// if we perform many allocations. By loading a preprocessed
// version of the wordlist, we can reduce the number of separate
// allocation, therefore improving performance by a lot.
// See 'scripts/preprocess_wordlist.{c|py}'
static void load_preprocessed_wordlist() {
    int input_file = open("assets/data.bin", O_RDONLY);

    struct stat input_stat;
    fstat(input_file, &input_stat);

    state.input_buffer = mmap(NULL, input_stat.st_size, PROT_READ, MAP_PRIVATE, input_file, 0);
    state.input_length = input_stat.st_size;

    if(state.input_buffer == MAP_FAILED) {
        fprintf(stderr, "Failed to open wordlist\n");
        exit(1);
    }

    char *pairs_start = state.input_buffer + 4;
    uint32_t total_pairs = *(uint32_t*)state.input_buffer;
    uint32_t offset = 0;

    state.key_value_pairs = calloc(total_pairs, sizeof(*state.key_value_pairs));

    for(uint32_t i = 0; i < total_pairs; i++) {
        struct KeyValuePair *pair = state.key_value_pairs + i;

        memcpy(&pair->key_size, pairs_start + offset, sizeof(pair->key_size));
        offset += 4;
        pair->key = pairs_start + offset;
        offset += pair->key_size;
        pair->was_deleted = false;
    }

    state.total_pairs = total_pairs;

    close(input_file);
}

static void close_preprocessed_wordlist(void) {
    munmap(state.input_buffer, state.input_length);
    free(state.key_value_pairs);
}

// see: http://www.isthe.com/chongo/tech/comp/fnv/
static uint32_t fnva1_hash_func(char *key, uint32_t key_size, void *udata) {
    uint32_t hash = 2166136261;

    for (uint32_t i = 0; i < key_size; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}

void test_hash_func(hm_hash_func hash_func, void *userdata) {
    hm_HashMap *map = hm_new(hash_func, userdata);
    struct KeyValuePair *pair;

    // Insert wordlist into the hashmap
    for(uint32_t i = 0; i < state.total_pairs; i++) {
        pair = state.key_value_pairs + i;
        hm_insert(map, pair->key, pair->key_size, &pair->value);
    }

    for(uint32_t i = 0; i < state.total_pairs; i++) {
        pair = state.key_value_pairs + i;
        assert(*(uint32_t*)hm_get(map, pair->key, pair->key_size) == pair->value);
    }

    for(uint32_t i = 100000; i < 200000; i++) {
        pair = state.key_value_pairs + i;
        pair->was_deleted = true;
        hm_delete(map, pair->key, pair->key_size);
    }

    for(uint32_t i = 50000; i < 150000; i++) {
        pair = state.key_value_pairs + i;
        void *value = hm_get(map, pair->key, pair->key_size);

        if(pair->was_deleted == true) {
            assert(value == NULL);
            continue;
        }

        assert(*(uint32_t*)value == pair->value);
    }

    hm_destroy(map);
}

#if 1

// An amazing StackOverflow thread: https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
int main(int argc, char **argv) {
    load_preprocessed_wordlist();
    puts("Processed wordlist!");

    test_hash_func(fnva1_hash_func, NULL);

    close_preprocessed_wordlist();

    return 0;
}

#else
int main(void) {
    hm_HashMap *map = hm_new(fnva1_hash_func, NULL);

    for(uint32_t i = 0; i < 1000; i ++) {
        char *value = "i do not exist";
        asssert(hm_get(map, value, strlen(value)) == NULL);
    }

    return 0;
}
#endif
