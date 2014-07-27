#include "lval.h"
#include <stdlib.h>

mem_heap*
heap_new(void) {
	mem_heap* heap = (mem_heap*)malloc(sizeof(mem_heap));
	heap->size = HEAP_INIT_SIZE;

	lval* head = (lval*)malloc(sizeof(lval));
	lval* n = head;
	memset(n, 0, sizeof(lval));

	for(int i = 0; i < heap->size; i++) {
		lval* tmp = (lval*)malloc(sizeof(lval));
		memset(tmp, 0, sizeof(lval));
		n->next = tmp;
		n = tmp;
	}

	n->next = NULL;
	heap->head = head;
	heap->next_free_val = heap->head;

	return heap;
}

void
heap_print(mem_heap* heap) {
	lval* n = heap->head;
	printf("HEAP\nsize:\t%d\ncontent:\n", heap->size);
	while(n){
		char c;
		switch(n->type) {
		case LVAL_UNDEF: c = '.'; break;
		case LVAL_ERR: c = 'E'; break;
		case LVAL_FUN: c = 'F'; break;
		case LVAL_NUM: c = 'N'; break;
		case LVAL_QEXPR: c = 'Q'; break;
		case LVAL_SEXPR: c = 'S'; break;
		case LVAL_STR: c = 'T'; break;
		case LVAL_SYM: c = 'A'; break;
		}
		putchar(c);
		n = n->next;
	}
	putchar('\n');
}

void
heap_del(mem_heap* heap) {
	lval* n = heap->head;

	// clean all lvalues
	while(n) {
		lval* tmp = n;
		n = n->next;
		lval_delp(tmp);
		free(tmp);
	}

	free(heap);
}

int
heap_resize(mem_heap* heap) {
	int old_size = heap->size;
	int new_size = (int)(old_size * HEAP_GROWTH_RATE);
	if(new_size > HEAP_MAX_SIZE) {
		return HEAP_MEM_OUT;
	}
	heap->size = new_size;

	lval* last = heap->head;
	while(last->next) { last = last->next; }

	lval* n = last;
	for(int i = old_size; i < new_size; i++) {
		lval* tmp = (lval*)malloc(sizeof(lval));
		memset(tmp, 0, sizeof(lval));
		n->next = tmp;
		n = tmp;
	}
	n->next = NULL;
	heap->next_free_val = last->next;

	return heap->size;
}

lval*
heap_next_free(mem_heap* heap){
	lval* v = heap->next_free_val;
	if(!v) { return v; }

	// checkout next free lvalue cell on the heap
	lval* n = v->next;
	while(n && n->type != LVAL_UNDEF) {
		n = n->next;
	}
	heap->next_free_val = n;

	return v;
}

lval*
lval_new(void) {
	mem_heap* h = HEAP;
	lval* v = heap_next_free(h);

	// if NULL was returned, perform heap resize
	while(!v) {
		if(heap_resize(h) == HEAP_MEM_OUT) { return NULL; }
		v = heap_next_free(h);
	}

	v->ref_count = 1;
	return v;
}

void
lval_del(lval* v) {
	if((--v->ref_count) <= 0) {
		// if reference counter hits 0, return lval to managed heap
		lval_delp(v);
	}
}

void
lval_delp(lval* v){
  switch(v->type){
  	case LVAL_UNDEF: break;
    case LVAL_NUM: break;
    case LVAL_STR: free(v->as.str); break;
    case LVAL_FUN:
    	if(!v->as.fun.builtin){
    		lenv_del(v->as.fun.env);
    		lval_del(v->as.fun.formals);
    		lval_del(v->as.fun.body);
    	}
    break;
    case LVAL_ERR: free(v->as.err); break;
    case LVAL_SYM: free(v->as.sym); break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for(int i=0; i < v->as.list.count; i++){
        lval_del(v->as.list.cell[i]);
      }
      free(v->as.list.cell);
    break;
  }

  // clear lvalue
  v->type = LVAL_UNDEF;
  v->hash = 0;
  v->ref_count = 0;
  //v->as = 0;

  // if pointer is before last free value on the heap, set free value as next to get
  if((v) < (HEAP->next_free_val)) {
	  HEAP->next_free_val = v;
  }
}

lval*
lval_cp(lval* v) {
	v->ref_count++;
	return v;
}

lval* lval_dcp(lval* v) {
	lval* x = lval_new();
	x->type = v->type;
	x->hash = v->hash;
	x->ref_count = 1;

	switch(v->type) {
		case LVAL_NUM: x->as.num = v->as.num; break;
		case LVAL_STR:
			x->as.str = (char*)malloc(strlen(v->as.str)+1);
			strcpy(x->as.str, v->as.str);
			break;
		case LVAL_FUN:
			if(v->as.fun.builtin){
				x->as.fun.builtin = v->as.fun.builtin;
			} else {
				x->as.fun.builtin = NULL;
				x->as.fun.env = lenv_copy(v->as.fun.env);
				x->as.fun.formals = lval_dcp(v->as.fun.formals);
				x->as.fun.body = lval_dcp(v->as.fun.body);
			}
			break;
		case LVAL_ERR: x->as.err = (char*)malloc(strlen(v->as.err)+1); strcpy(x->as.err, v->as.err); break;
		case LVAL_SYM: x->as.sym = (char*)malloc(strlen(v->as.sym)+1); strcpy(x->as.sym, v->as.sym); break;

		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->as.list.count = v->as.list.count;
			x->as.list.cell = (lval**)malloc(sizeof(lval*) * x->as.list.count);
			for(int i = 0; i < x->as.list.count; i++) {
				x->as.list.cell[i] = lval_dcp(v->as.list.cell[i]);
			}
			break;
	}

	return x;
}
