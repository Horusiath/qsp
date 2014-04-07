#ifndef LVAL_H
#define LVAL_H


/* Create Enumeration of Possible Error Types */
enum { 
	LERR_DIV_ZERO, 
	LERR_BAD_OP, 
	LERR_BAD_NUM 
};

/* Create Enumeration of Possible lval Types */
enum { 
	LVAL_NUM, 
	LVAL_ERR,
	LVAL_SYM,
	LVAL_SEXPR 
};

/* Declare New lval Struct */
typedef struct {
  int type;
  long num;
  char* err;
  char* sym;

  int count;
  struct lval** cell;
} lval;

lval* lval_num(long x);
lval* lval_err(char* x);
lval* lval_sym(char* s);
lval* lval_sexpr(void);

lval* lval_add(lval* sexpr, lval* x);
void lval_del(lval* v);

void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);

lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_eval(lval* v);
lval* lval_eval_sexpr(lval* v);

#endif
