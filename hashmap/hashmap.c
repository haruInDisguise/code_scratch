#include "hashmap.h"

#define INITIAL_SIZE 51

#define UPSIZE_AT_PERCENT 75
#define UPSIZE_FACTOR 2

#define DOWNSIZE_AT_PERCENT 50
#define DOWNSIZE_FACTOR 1.5

#define DOUBLEHASH_TIMEOUT 120

static uint32_t find_next_prime(uint32_t value) {
    uint32_t prime = 0;

    return prime;
}

static inline uint32_t get_hash(hm_HashMap *map, char *key, uint32_t attempt) {
    uint32_t hash = map->hash_func(key, map->seed_one);

    if (attempt > 0) {
        hash = hash + attempt * (map->hash_func(key, map->seed_two) + 1);
    }

    return hash;
}

static hm_Bucket *find_empty(hm_HashMap *map, char *key, uint32_t key_length) {
    for (uint32_t attempt = 0; attempt < DOUBLEHASH_TIMEOUT; attempt++) {
        uint32_t index = get_hash(map, key, attempt) % map->total_capacity;

        if (map->buckets[index].occupied == false)
            return &map->buckets[index];
    }

    fprintf(stderr, "find_empty(): Double Hash Timeout! :^( Key: %s \n", key);

    return NULL;
}


static hm_Bucket *find_occupied(hm_HashMap *map, char *key, uint32_t key_length) {
    for (uint32_t attempt = 0; attempt < DOUBLEHASH_TIMEOUT; attempt++) {
        uint32_t index = get_hash(map, key, attempt) % map->total_capacity;

        if (map->buckets[index].occupied == true &&
            memcmp(map->buckets[index].key, key, key_length) == 0)
            return &map->buckets[index];
    }

    fprintf(stderr, "find_index(): Double Hash Timeout! :^( Key: %s \n", key);

    return NULL;
}

static void rehash(hm_HashMap *map) {
    hm_HashMap new_map = {
        .used_capacity = map->used_capacity,
        .total_capacity = map->total_capacity * UPSIZE_FACTOR,
        .buckets = calloc(map->total_capacity * UPSIZE_FACTOR, sizeof(*map->buckets)),
        .seed_one = map->seed_one,
        .seed_two = map->seed_two,
        .hash_func = map->hash_func,
    };

    memcpy(map, &new_map, sizeof(new_map));
}

void hm_insert(hm_HashMap *map, char *key, void *value) {
    if((map->used_capacity + 1) * 100 / map->total_capacity >= UPSIZE_AT_PERCENT)
        rehash(map);

    hm_Bucket *bucket = find_empty(map, key, strlen(key));

    if(bucket == NULL) {
        exit(1);
    }

    *bucket = (hm_Bucket){
        .value = value,
        .key = key,
        .key_length = strlen(key),
        .occupied = true,
    };

    map->used_capacity ++;
}

void hm_delete(hm_HashMap *map, char *key) {
    hm_Bucket *bucket = find_occupied(map, key, strlen(key));

    if (bucket->occupied == true) return;

    memset(bucket, 0, sizeof(*bucket));
    map->used_capacity--;
}

// FIXME: The value cannot be NULL
void *hm_get(hm_HashMap *map, char *key) {
    hm_Bucket *bucket = find_occupied(map, key, strlen(key));

    if (bucket->occupied == false) return NULL;

    return bucket->value;
}

hm_HashMap *hm_new(hm_hash_func hash_func, uint64_t seed_one, uint64_t seed_two) {
    hm_HashMap *map = calloc(1, sizeof(*map));

    *map = (hm_HashMap){
        .buckets = calloc(INITIAL_SIZE, sizeof(*map->buckets)),
        .hash_func = hash_func,
        .total_capacity = INITIAL_SIZE,
        .seed_one = seed_one,
        .seed_two = seed_two,
    };

    return map;
}

void hm_destroy(hm_HashMap *map) {
    free(map->buckets);
    free(map);
}
