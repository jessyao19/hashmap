#ifndef HASHMAP_PROJECT_H
#define HASHMAP_PROJECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define NUMBER_BUCKETS 10

struct node {
	void* key;
	void* value;
	struct node* next;
};

struct hash_map {
	struct node** array;
	pthread_mutex_t bucket_mutex[NUMBER_BUCKETS]; //an array of mutexes in stack
	size_t (*hash)(void*); //function pointer
	int (*cmp)(void*, void*);
	void (*key_destruct)(void*);
	void (*value_destruct)(void*);
};

int compare(void* obj1, void* obj2);

size_t hash(void* input);

void key_destroy(void* thing);

void value_destroy(void* thing);

struct hash_map* hash_map_new(size_t (*hash)(void*), int (*cmp)(void*, void*),
	void (*key_destruct)(void*), void (*value_destruct)(void*));

void hash_map_put_entry_move(struct hash_map* map, void* k, void* v);

void hash_map_remove_entry(struct hash_map* map, void* k);

void* hash_map_get_value_ref(struct hash_map* map, void* k);

void hash_map_destroy(struct hash_map* map);

void print_all(struct hash_map* hashmap);

#endif