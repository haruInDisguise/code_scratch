#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define INITIAL_SIZE 128
#define UPSIZE_AT_PERCENT 75
#define DOWNSIZE_AT_PERCENT 50

typedef uint32_t (*hm_hash_func)(char* value, uint32_t seed);

typedef struct {
    uint8_t occupied;
    void *value;
    char *key;
} hm_Bucket;

typedef struct {
    uint32_t total_capacity;
    uint32_t used_capacity;
    uint32_t seed_one;
    uint32_t seed_two;

    hm_hash_func hash_func;
    hm_Bucket *buckets;
} hm_HashMap;

void hm_insert(hm_HashMap *map, char *key, void *value);
void *hm_get(hm_HashMap *map, char *key);

hm_HashMap *hm_new(hm_hash_func hash_func, uint64_t seed_one, uint64_t seed_two);

#endif
