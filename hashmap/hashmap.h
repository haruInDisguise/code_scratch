#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <stdbool.h>
#include <stdint.h>

//#define HT_DEBUG

typedef uint64_t (*hm_hash_func)(char *value, uint32_t key_size, void *userdata);

typedef struct {
    uint32_t key_size;
    void *value;
    char *key;
} hm_Bucket;

typedef struct {
    uint32_t total_capacity;
    uint32_t used_capacity;
    hm_hash_func hash_func;
    void *userdata;
    hm_Bucket *buckets;
} hm_HashMap;

void hm_insert(hm_HashMap *map, char *key, uint32_t key_size, void *value);
void *hm_get(hm_HashMap *map, char *key, uint32_t key_size);
void hm_delete(hm_HashMap *map, char *key, uint32_t key_size);

hm_HashMap *hm_new(hm_hash_func hash_func, void *udata);
void hm_destroy(hm_HashMap *map);

#endif
