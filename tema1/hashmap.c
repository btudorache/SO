#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_SIZE 80

typedef struct Node {
    char key[KEY_SIZE];
    char value[KEY_SIZE]; 
} Node;

typedef struct Hashmap {
    int size;
    Node* list;
} Hashmap;

/**
 * taken from http://www.cse.yorku.ca/~oz/hash.html
 */
static unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

Hashmap* init_hashmap(int size) {
    Hashmap* hashmap = malloc(sizeof(Hashmap));
    hashmap->size = size;
    hashmap->list = calloc(size, sizeof(Node));

    return hashmap;
}

int has_key(Hashmap* hashmap, char* key) {
    unsigned long hash_value = hash((unsigned char *)key);
    int bucket = (int)(hash_value % hashmap->size);

    if (strcmp(hashmap->list[bucket].key, "") == 0) {
        return 0;
    } else {
        return 1;
    }
}

void* get_value(Hashmap* hashmap, char* key) {    
    unsigned long hash_value = hash((unsigned char *)key);
    int bucket = (int)(hash_value % hashmap->size);
    Node* list = hashmap->list;
    int i = 0;
    if (strcmp(list[bucket].key, key) == 0) {
        return list[bucket].value;
    }

    for (i = bucket + 1; i < hashmap->size; i++) {
        if (strcmp(list[i].key, key) == 0) {
            return list[i].value;
        }
    }

    for (i = 0; i < bucket; i++) {
        if (strcmp(list[i].key, key) == 0) {
            return list[i].value;
        }
    }

    return NULL;
}

void put_value(Hashmap* hashmap, char* key, void* value) {
    unsigned long hash_value = hash((unsigned char *)key);
    int bucket = (int)(hash_value % hashmap->size);
    int actual_bucket = -1;
    Node* list = hashmap->list;
    int i = 0;
    if (strcmp(list[bucket].key, "") == 0 || 
        strcmp(list[bucket].key, key) == 0) {
        actual_bucket = bucket;
    } else {
        for (i = bucket + 1; i < hashmap->size; i++) {
            if (strcmp(list[i].key, "") == 0 || 
                strcmp(list[i].key, key) == 0) {
                actual_bucket = i;
                break;
            }
        }

        if (actual_bucket == -1) {
            for (i = 0; i < hashmap->size; i++) {
                if (strcmp(list[i].key, "") == 0 || 
                    strcmp(list[i].key, key) == 0) {
                    actual_bucket = i;
                    break;
                }
            }
        }

        if (actual_bucket == -1) {
            printf("Hashmap is full\n");
            exit(EXIT_FAILURE);
        }
    }
    
    if (strcmp(list[actual_bucket].key, key) == 0) {
        strcpy(list[actual_bucket].value, value);
    } else {
        strcpy(list[actual_bucket].key, key);
        strcpy(list[actual_bucket].value, value);
    }
}

void free_hashmap(Hashmap* hashmap) {
    free(hashmap->list);
    free(hashmap);
};