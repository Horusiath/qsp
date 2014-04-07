#include "lval.h"
#include <stdio.h>

/* Create a new number type lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Create a new error type lval */
lval* lval_err(char* msg) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(msg) + 1);
  strcpy(v->err, msg);
  return v;
}

lval* lval_sym(char* s){
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void){
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;

	return v;
}

lval* lval_add(lval* e, lval* x) {
  e->count++;
  e->cell = realloc(e->cell, sizeof(lval*) * e->count);
  e->cell[e->count-1] = x;
  return e;
}

void lval_del(lval* v){
  switch(v->type){
    case LVAL_NUM: break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_QEXPR:
    case LVAL_SEXPR: 
      for(int i=0; i < v->count; i++){
        lval_del(v->cell[i]);
      }
      free(v->cell);
    break;
  }

  free(v);
}

/* Print an "lval" */
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
  }
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);

  for(int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if(i != (v->count-1)){
        putchar(' ');
    }
  }

  putchar(close);
}

/* Print an "lval" followed by a newline */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];

    // shift memory
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);

    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

#define LASSERT(args, cond, err) if(!(cond)) { lval_del(args); return lval_err(err); }

lval* builtin_head(lval* a) {
	// check error
	LASSERT(a, (a->count == 1), "Function 'head' passed too many arguments!")

	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'head' passed incorrect types!")
	LASSERT(a, (h->count != 0), "Function 'head' passed {}!")

	h = lval_take(a, 0);

	// delete all non-head elements
	while(h->count > 1) {
		lval_del(lval_pop(h, 1));
	}
	return h;
}

lval* builtin_tail(lval* a) {
	// check for errors
	LASSERT(a, (a->count == 1), "Function 'head' passed too many arguments!")

	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'head' passed incorrect types!")
	LASSERT(a, (h->count != 0), "Function 'head' passed {}!")

	//take first
	h = lval_take(a, 0);

	// delete first element and return
	lval_del(lval_pop(h, 0));

	return h;
}

lval* builtin_list(lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lval* a) {
	LASSERT(a, (a->count == 1), "Function 'eval' passed too many parameters!")
	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!")

	h = lval_take(a, 0);
	h->type = LVAL_SEXPR;
	return lval_eval(h);
}

lval* lval_join(lval* x, lval* y){
	while(y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

lval* builtin_join(lval* a) {
	for(int i = 0; i < a->count; i++) {
		lval* h = a->cell[i];
		LASSERT(a, (h->type == LVAL_QEXPR), "Function 'join' passed invalid type");
	}

	lval* x = lval_pop(a, 0);

	while(a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);
	return x;
}

lval* builtin_op(lval* a, char* op) {

    // ensure all arguments are numbers
    for(int i = 0; i < a->count; i++) {
    	lval* o = a->cell[i];
        if(o->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non number");
        }
    }

    lval* x = lval_pop(a, 0);

    // try to perform unary negation
    if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

    // for all elements
    while(a->count > 0) {
        lval* y = lval_pop(a, 0);

        // perform operation
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if(y->num == 0) {
                lval_del(x);
                lval_del(y);
                lval_del(a);
                x = lval_err("Division by zero!"); break;
            } else {
                x->num /= y->num;
            }
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval* builtin(lval* a, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(a); }
	if (strcmp("head", func) == 0) { return builtin_head(a); }
	if (strcmp("tail", func) == 0) { return builtin_tail(a); }
	if (strcmp("join", func) == 0) { return builtin_join(a); }
	if (strcmp("eval", func) == 0) { return builtin_eval(a); }
	if (strcmp("+-*/", func)) { return builtin_op(a, func); }

	lval_del(a);
	return lval_err("Unknown function!");
}

lval* lval_eval_sexpr(lval* v) {

    // evaluate children
    for(int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    // error check
    for(int i = 0; i < v->count; i++) {
    	lval* o = v->cell[i];
        if(o->type == LVAL_ERR) { return lval_take(v, i); }
    }

    // empty expression
    if (v->count == 0) { return v; }

    // single expression
    if (v->count == 1) { return lval_take(v, 0); }

    // ensure first elem is symbol
    lval* s = lval_pop(v, 0);
    if (s->type != LVAL_SYM) {
        lval_del(s);
        lval_del(v);
        return lval_err("expression does not start with a symbol");
    }

    lval* res = builtin(v, s->sym);
    lval_del(s);
    return res;
}

lval* lval_eval(lval *v){
    if(v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

    return v;
}
