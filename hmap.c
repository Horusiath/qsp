#include "hmap.h"
#include <stdlib.h>

#define HASH_INIT_SIZE 4
#define HASH_GROWT_RATE 2

#define HASH_RES(h, key) key % h->cap

hmap* hmap_new(void) {
	 hmap* h = (hmap*)malloc(sizeof(hmap));
	 h->slots = (hslot*)malloc(sizeof(hslot) * HASH_INIT_SIZE);
	 memset(h->slots, 0, sizeof(hslot) * HASH_INIT_SIZE);
	 h->cap = HASH_INIT_SIZE;
	 h->len = 0;

	 return h;
}

void hmap_del(hmap* h) {
	free(h->slots);
	free(h);
}

// hashing function for an int
unsigned int hmap_int_h(int key) {
	key += (key << 12);
	key ^= (key >> 22);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 2);
	key += (key << 7);
	key ^= (key >> 12);

	/* Knuth's Multiplicative Method */
	key = (key >> 3) * 2654435761;

	return key;
}

// hashing function for a string
unsigned int hmap_str_h(char* s) {
	int hash = 0, n = strlen(s);
	for (int i = 0; i < n; i++) {
		hash = 31*hash + s[i];
	}
	return hash;
}


int hmap_h(hmap* h, int hash) {
	if(h->len == h->cap) { return HASH_FULL; }

	int c = HASH_RES(h, hash);
	for(int i = 0; i < h->cap; i++) {
		hslot s = h->slots[c];
		if(s.used == 0) { return c; }
		if(s.hash == hash && s.used == 1) { return c; }

		c = (c + 1) % h->cap;
	}

	return HASH_FULL;
}

int hmap_put(hmap* h, int hash, void* val) {
	int i = hmap_h(h, hash);
	while(i == HASH_FULL) {
		if(hmap_rehash(h) == HASH_MEM_OUT) { return HASH_MEM_OUT; }
		i = hmap_h(h, hash);
	}

	// set in data
   	h->slots[i].val = val;
	h->slots[i].used = 1;
	h->slots[i].hash = hash;

	h->len++;

	return HASH_OK;
}

int hmap_rehash(hmap* h) {
	int old_size = h->cap;
	int new_size = (int)(old_size * HASH_GROWT_RATE);
	hslot* tmp = (hslot*)malloc(sizeof(hslot) * new_size);
	if(!tmp) { return HASH_MEM_OUT; }

	memset(tmp, 0, new_size);
	hslot* curr = h->slots;
	h->slots = tmp;
	h->cap = new_size;
	h->len = 0;

	for(int i = 0; i < old_size; i++) {
		int status = hmap_put(h, curr[i].hash, curr[i].val);
		if(status != HASH_OK) return status;
	}

	free(curr);
	return HASH_OK;
}

void* hmap_get(hmap* h, int hash) {
	int c = HASH_RES(h, hash);

	// linear probing
	for(int i = 0; i < h->cap; i++) {
		hslot s = h->slots[c];
		if(s.hash == hash && s.used == 1) {
			return s.val;
		}
		c = (c + 1) % h->cap;
	}

	return NULL;
}

int hmap_rem(hmap* h, int hash) {
	int c = HASH_RES(h, hash);

	// linear probing
	for(int i = 0; i < h->cap; i++) {
		hslot s = h->slots[c];
		if(s.hash == hash && s.used == 1) {
			s.used = 0;
			s.hash = 0;
			s.val = NULL;

			h->len--;
			return HASH_OK;
		}
		c = (c + 1) % h->cap;
	}

	return HASH_MISSING;
}
