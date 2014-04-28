#ifndef LVAL_H
#define LVAL_H

#include "hmap.h"

#define LASSERT(args, cond, fmt, ...) 				\
	if(!(cond)) { 									\
		lval* err = lval_err(fmt, ##__VA_ARGS__);	\
		lval_del(args);								\
		return err;									\
	}

#define LASSERT_TYPE(func, args, index, expect)											\
	LASSERT(args, (args->as.list.cell[index]->type == expect), 							\
		"Function '%s' passed incorrect type for argument %i. Got %s, expected %s.",	\
		func, index, ltype_name(args->as.list.cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num)													\
	LASSERT(args, (args->as.list.count == num),											\
		"Function '%s' passed incorrect number of arguments. Got %i, expected %i.",		\
		func, args->as.list.count, num)

#define LASSERT_NOT_EMPTY(func, args, index) 			\
	LASSERT(args, (args->as.list.cell[index]->as.list.count != 0),		\
		"Function '%s' passed {} for argument %i.",		\
		func, index)

struct mem_heap;
struct lval;
struct lenv;
struct lfun;
struct llist;
typedef struct mem_heap mem_heap;
typedef struct lval lval;
typedef struct lenv lenv;
typedef struct lfun lfun;
typedef struct llist llist;

typedef lval* (*lbuiltin)(lenv*, lval*);

/* Create Enumeration of Possible Error Types */
enum { 
	LERR_DIV_ZERO, 
	LERR_BAD_OP, 
	LERR_BAD_NUM 
};

/* Create Enumeration of Possible lval Types */
enum { 
	LVAL_UNDEF = 0,
	LVAL_NUM, 
	LVAL_STR,
	LVAL_FUN,
	LVAL_ERR,
	LVAL_SYM,
	LVAL_QEXPR,
	LVAL_SEXPR 
};

#define HEAP_INIT_SIZE 		1000
#define HEAP_GROWTH_RATE 	2
#define HEAP_MAX_SIZE		100000

enum {
	HEAP_MEM_OUT = -1
};

/* managed memory heap */
struct mem_heap {
	int		size;
	lval* 	head;
	lval* 	next_free_val;
};

extern mem_heap* HEAP;

/* lambda function struct */
struct lfun {
	lbuiltin 	builtin;
	lenv* 		env;
	lval* 		formals;
	lval* 		body;
};

struct llist {
	int 	count;
	lval** 	cell;
};

struct lval {
  int type;
  int hash;
  int ref_count;
  lval* next;

  union {
	  char* err;
	  char* sym;
	  char* str;
	  long 	num;
	  lfun 	fun;
	  llist list;
  } as;
};

struct lenv {
  lenv* par;
  hmap* map;
};

char * ltype_name(int t);

lval* lval_num(long x);
lval* lval_str(char* s);
lval* lval_fun(lbuiltin func);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_qexpr(void);

/* Creates a new managed heap. */
mem_heap* heap_new(void);

/* Prints current heap content. */
void heap_print(mem_heap* heap);

/* Deletes a managed heap with all of lvalues inside. */
void heap_del(mem_heap* heap);

/* Allocates a new lvalue from managed heap of undefined type */
lval* lval_new(void);

/* Adds a [x] to list [sexpr] */
lval* lval_add(lval* sexpr, lval* x);

/* Creates a shallow copy of lvalue. */
lval* lval_cp(lval* c);

/* Creates a deep copy of lvalue. Unlike lval_cp this one allocates a completely new lvalue and copies all of it's inner states. */
lval* lval_dcp(lval* c);

/* Deletes a lvalue. This functions frees a lvalue back to heap only if reference counter hits zero. */
void lval_del(lval* v);
lval* lval_call(lenv* e, lval* f, lval* a);

void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
void lenv_print(lenv* e);

lval* lval_eval(lenv* env, lval* v);
lval* lval_eval_sexpr(lenv* env, lval* v);

lenv* lenv_new(void);
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* v, lval* k);
void lenv_def(lenv* e, lval* v, lval* k);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

unsigned int hmap_list_h(int n, lval* s);

#endif
