#ifndef LVAL_H
#define LVAL_H

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

struct lval;
struct lenv;
struct lfun;
struct llist;
typedef struct lval lval;
typedef struct lenv lenv;
typedef struct lfun lfun;
typedef struct llist llist;

typedef lval* (*lbuiltin)(lenv*, lval*);

typedef struct {
	int hash;
	int used;
	lval* val;
} hslot;

typedef struct {
	int cap;
	int len;
	hslot* slots;
} hmap;

/* Create Enumeration of Possible Error Types */
enum { 
	LERR_DIV_ZERO, 
	LERR_BAD_OP, 
	LERR_BAD_NUM 
};

/* Create Enumeration of Possible lval Types */
enum { 
	LVAL_NUM, 
	LVAL_STR,
	LVAL_FUN,
	LVAL_ERR,
	LVAL_SYM,
	LVAL_QEXPR,
	LVAL_SEXPR 
};

enum {
	HASH_MISSING = -3,
	HASH_FULL = -2,
	HASH_MEM_OUT = -1,
	HASH_OK = 0
};

/* lambda function struct */
struct lfun {
	lbuiltin builtin;
	lenv* env;
	lval* formals;
	lval* body;
};

struct llist {
	int 	count;
	lval** 	cell;
};


struct lval {
  int type;
  int hash;

  union {
	  char* err;
	  char* sym;
	  char* str;
	  long num;
	  lfun fun;
	  llist list;
  } as;
};

struct lenv {
  lenv* par;
  hmap* map;
};

lval* lval_num(long x);
lval* lval_str(char* s);
lval* lval_fun(lbuiltin func);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_qexpr(void);

lval* lval_add(lval* sexpr, lval* x);
lval* lval_copy(lval* c);
void lval_del(lval* v);

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

hmap* hmap_new(void);
void hmap_del(hmap* h);
int hmap_put(hmap* h, int key, lval* val);
lval* hmap_get(hmap* h, int key);
int hmap_rem(hmap* h, int key);

unsigned int hmap_int_h(int key);
unsigned int hmap_str_h(char* s);
unsigned int hmap_list_h(int n, lval* s);

#endif
