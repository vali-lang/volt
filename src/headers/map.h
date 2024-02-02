
#ifndef H_MAP
#define H_MAP

#include "typedefs.h"

struct Map {
    Allocator *alc;
    Array *keys;
    Array *values;
    int find_start;
};

Map *map_make(Allocator *alc);
bool map_contains(Map *map, char *key);
void *map_get(Map *map, char *key);
bool map_unset(Map *map, char *key);
void map_set(Map *map, char *key, void *value);
void map_print_keys(Map *map);

#endif