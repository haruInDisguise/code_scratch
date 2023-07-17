#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t (*hm_hash_func)(char *value, uint32_t seed);

typedef struct {
    uint32_t key_length;
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
void hm_remove(hm_HashMap *map, char *key);

hm_HashMap *hm_new(hm_hash_func hash_func, uint64_t seed_one, uint64_t seed_two);
void hm_destroy(hm_HashMap *map);

#endif
