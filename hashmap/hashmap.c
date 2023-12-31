#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

#define INITIAL_SIZE 127
#define DELETED_MARK ((uint32_t *)-1)

#define UPSIZE_AT_PERCENT 70
#define DOWNSIZE_AT_PERCENT 50

#define DOUBLEHASH_TIMEOUT 120

static struct {
    uint32_t collisions;
} state;

// For DoubleHashing, I use the famous FNV-a1 hashing algorithm.
// see: http://www.isthe.com/chongo/tech/comp/fnv/
static inline uint32_t double_hashing_func(char *key, uint32_t key_size) {
    uint32_t hash = 216613626;

    for(uint32_t i = 0; i < key_size; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}

static inline uint32_t get_hash(hm_HashMap *map, char *key, uint32_t key_size, uint32_t attempt) {
    uint32_t hash = map->hash_func(key, key_size, map->userdata);

    if (attempt > 0) {
        hash = hash + attempt * (double_hashing_func(key, key_size) + 1);
    }

    return hash % map->total_capacity;
}

// Find an empty or deleted bucket
static hm_Bucket *find_empty(hm_HashMap *map, char *key, uint32_t key_size) {
    for (uint32_t attempt = 0; attempt < DOUBLEHASH_TIMEOUT; attempt++) {
        uint32_t index = get_hash(map, key, key_size, attempt);

        if (map->buckets[index].value == DELETED_MARK || map->buckets[index].value == NULL) {
            return &map->buckets[index];
        }
    }

    fprintf(stderr, "find_empty(): Double Hash Timeout! :^( Key: %sn", key);
    exit(1);
}

// Find an occupied bucket, skimming past deleted entries
static hm_Bucket *find_occupied(hm_HashMap *map, char *key, uint32_t key_size) {
    for (uint32_t attempt = 0; attempt < DOUBLEHASH_TIMEOUT; attempt++) {
        uint32_t index = get_hash(map, key, key_size, attempt) % map->total_capacity;

        if (map->buckets[index].value == NULL)
            return NULL;
        else if (map->buckets[index].value == DELETED_MARK) {
            continue;
        }
        else if (memcmp(map->buckets[index].key, key, key_size) == 0)
            return &map->buckets[index];
    }

    fprintf(stderr, "find_empty(): Double Hash Timeout! :^( Key: %s \n", key);
    exit(1);
}

static void resize(hm_HashMap *map, uint32_t new_size) {
    hm_HashMap new_map = {
        .total_capacity = new_size,
        .buckets = calloc(new_size, sizeof(*map->buckets)),
        .hash_func = map->hash_func,
        .userdata = map->userdata,
    };

    for (uint32_t i = 0; i < map->total_capacity; i++) {
        // We also copy deleted buckets
        if (map->buckets[i].value != NULL)
            hm_insert(&new_map, map->buckets[i].key, map->buckets[i].key_size, map->buckets[i].value);
    }

    free(map->buckets);

    memcpy(map, &new_map, sizeof(new_map));
}

void *hm_get(hm_HashMap *map, char *key, uint32_t key_size) {
    hm_Bucket *bucket = find_occupied(map, key, key_size);

    if (bucket == NULL) return NULL;

    return bucket->value;
}

void hm_insert(hm_HashMap *map, char *key, uint32_t key_size, void *value) {
    if ((map->used_capacity + 1) * 100 / map->total_capacity >= UPSIZE_AT_PERCENT)
        resize(map, map->total_capacity * 4);

    hm_Bucket *bucket = find_empty(map, key, key_size);

    *bucket = (hm_Bucket){
        .value = value,
        .key = key,
        .key_size = key_size,
    };

    map->used_capacity++;
}

void hm_delete(hm_HashMap *map, char *key, uint32_t key_size) {
    hm_Bucket *bucket = find_occupied(map, key, key_size);

    if (bucket == NULL) return;

    *bucket = (hm_Bucket){
        .value = DELETED_MARK,
    };

    map->used_capacity--;
}

hm_HashMap *hm_new(hm_hash_func hash_func, void *userdata) {
    hm_HashMap *map = calloc(1, sizeof(*map));

    *map = (hm_HashMap){
        .buckets = calloc(INITIAL_SIZE, sizeof(*map->buckets)),
        .hash_func = hash_func,
        .total_capacity = INITIAL_SIZE,
        .userdata = userdata,
    };

    return map;
}

void hm_destroy(hm_HashMap *map) {
    free(map->buckets);
    free(map);
}
