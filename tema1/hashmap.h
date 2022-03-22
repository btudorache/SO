#ifndef HASHMAP_H_
#define HASHMAP_H_ 1

typedef struct Hashmap Hashmap;

Hashmap* init_hashmap();

int has_key(Hashmap* hashmap, char* key);

void* get_value(Hashmap* hashmap, char* key);

void put_value(Hashmap* hashmap, char* key, void* value);

void free_hashmap(Hashmap* hashmap);

#endif /* HASHMAP_H_ */
