#include "hash_map.h"

#define INITIAL_SIZE 128
#define UPSIZE_AT_PERCENT 75
#define DOWNSIZE_AT_PERCENT 50
#define DOUBLEHASH_TIMEOUT 80

static uint32_t string_hash_func(char *value, uint32_t seed) {
    uint32_t key = 0;

    for(uint32_t i = 0; i < strlen(value); i++) {
        key += (value[i] * seed + i);
    }

    return key;
}

static uint32_t get_hash(hm_HashMap *map, char *key, uint32_t attempt) {
    uint32_t hash = map->hash_func(key, map->seed_one);

    if(attempt > 0) {
        hash = hash + attempt * (map->hash_func(key, map->seed_two) + 1);
    }

    return hash;
}

static uint32_t get_index(hm_HashMap *map, char *key) {
    uint32_t attempt = 0;
    uint32_t index = 0;

    while(attempt < DOUBLEHASH_TIMEOUT) {
        index = get_hash(map, key, attempt) % map->total_capacity;
        if (map->buckets[index].occupied == false) {
            break;
        }

        printf("Merge conflict! Attempting double hashing... Key: %s, Attempt: %u\n", key, attempt);

        attempt ++;
    }

    return index;
}

static void rehash(hm_HashMap *map) {
    // TODO
}

void hm_insert(hm_HashMap *map, char *key, void *value) {
    uint32_t index = get_index(map, key);

    if(map->buckets[index].occupied == true) {
        fprintf(stderr, "Merge conflict! \"%s\"\n", key);
        exit(1);
    }

    // TODO: Resize

    map->buckets[index] = (hm_Bucket) {
        .key = key,
        .value = value,
        .occupied = true,
    };
}

void *hm_get(hm_HashMap *map, char *key) {
    return map->buckets[get_index(map, key)].value;
}

hm_HashMap *hm_new(hm_hash_func hash_func, uint64_t seed_one, uint64_t seed_two) {
    hm_HashMap *map = calloc(1, sizeof(*map));

    *map = (hm_HashMap) {
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


