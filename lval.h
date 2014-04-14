#ifndef LVAL_H
#define LVAL_H

#define LASSERT(args, cond, fmt, ...) 				\
	if(!(cond)) { 									\
		lval* err = lval_err(fmt, ##__VA_ARGS__);	\
		lval_del(args);								\
		return err;									\
	}

#define LASSERT_TYPE(func, args, index, expect)											\
	LASSERT(args, (args->cell[index]->type == expect), 									\
		"Function '%s' passed incorrect type for argument %i. Got %s, expected %s.",	\
		func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num)													\
	LASSERT(args, (args->count == num),													\
		"Function '%s' passed incorrect number of arguments. Got %i, expected %i.",		\
		func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) 			\
	LASSERT(args, (args->cell[index]->count != 0),		\
		"Function '%s' passed {} for argument %i.",		\
		func, index)

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval* (*lbuiltin)(lenv*, lval*);

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

/* Declare New lval Struct */
struct lval {
  int type;

  /* basic data */
  char* err;
  char* sym;

  /* strings */
  char* str;

  /* builtin functions */
  lbuiltin builtin;

  /* lambda functions */
  lenv* env;
  lval* formals;
  lval* body;

  /* values */
  long num;

  /* expression */
  int count;
  struct lval** cell;
};

struct lenv {
  lenv* par;
  int count;
  char** syms;
  lval** vals;
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

lval* lval_eval(lenv* env, lval* v);
lval* lval_eval_sexpr(lenv* env, lval* v);

lenv* lenv_new(void);
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* v, lval* k);
void lenv_def(lenv* e, lval* v, lval* k);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

#endif
