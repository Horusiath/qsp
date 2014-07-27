#ifndef HMAP_H
#define HMAP_H

enum {
	HASH_MISSING = -3,
	HASH_FULL = -2,
	HASH_MEM_OUT = -1,
	HASH_OK = 0
};

typedef struct {
	int hash;
	int used;
	void* val;
} hslot;

typedef struct {
	int cap;
	int len;
	hslot* slots;
} hmap;


hmap* hmap_new(void);
void hmap_del(hmap* h);
int hmap_put(hmap* h, int key, void* val);
void* hmap_get(hmap* h, int key);
int hmap_rem(hmap* h, int key);

unsigned int hmap_int_h(int key);
unsigned int hmap_str_h(char* s);

#endif
