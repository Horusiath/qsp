#include "lval.h"
#include <stdio.h>

/* Create a new number type lval */
lval* lval_num(long x) {
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_fun(lbuiltin func) {
	  lval* v = (lval*)malloc(sizeof(lval));
	  v->type = LVAL_FUN;
	  v->fun = func;
	  return v;
}

/* Create a new error type lval */
lval* lval_err(char* msg) {
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = (char*)malloc(strlen(msg) + 1);
  strcpy(v->err, msg);
  return v;
}

lval* lval_sym(char* s){
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = (char*)malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void){
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

lval* lval_qexpr(void) {
	lval* v = (lval*)malloc(sizeof(lval));
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
    case LVAL_FUN: break;
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
    case LVAL_FUN: printf("<function>"); break;
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

lval* lval_copy(lval* v) {
	lval* x = (lval*)malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type) {
		case LVAL_NUM: x->num = v->num; break;
		case LVAL_FUN: x->fun = v->fun; break;

		case LVAL_ERR: x->err = (char*)malloc(strlen(v->err)+1); strcpy(x->err, v->err); break;
		case LVAL_SYM: x->sym = (char*)malloc(strlen(v->sym)+1); strcpy(x->sym, v->sym); break;

		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->count = v->count;
			x->cell = (lval**)malloc(sizeof(lval*) * x->count);
			for(int i = 0; i < x->count; i++) {
				x->cell[i] = lval_copy(v->cell[i]);
			}
			break;
	}

	return x;
}

lenv* lenv_new(void) {
	lenv* e = (lenv*)malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;
	return e;
}

void lenv_del(lenv* e) {
	for(int i = 0; i < e->count; i++){
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}

	free(e->syms);
	free(e->vals);
	free(e);
}

lval* lenv_get(lenv* e, lval* k) {
	for(int i = 0; i < e->count; i++) {
		if(strcmp(e->syms[i], k->sym) == 0) { return lval_copy(e->vals[i]); }
	}

	return lval_err("unbound symbol!");
}

void lenv_put(lenv* env, lval* key, lval* val) {
	// check if symbol already exists (don't insert duplicated values)
	for(int i = 0; i < env->count; i++) {
		if(strcmp(env->syms[i], key->sym) == 0) {
			lval_del(env->vals[i]);
			env->vals[i] = lval_copy(val);
			return;
		}
	}

	// if not entry found, allocate space for new one
	int c = ++env->count;
	env->vals = (lval**)realloc(env->vals, sizeof(lval*) * c);
	env->syms = (char**)realloc(env->syms, sizeof(char*) * c);

	// copy contents
	env->vals[c-1] = lval_copy(val);
	env->syms[c-1] = (char*)malloc(strlen(key->sym) + 1);
	strcpy(env->syms[c-1], key->sym);
}

#define LASSERT(args, cond, err) if(!(cond)) { lval_del(args); return lval_err(err); }

lval* builtin_head(lenv* e, lval* a) {
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

lval* builtin_tail(lenv* e, lval* a) {
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

lval* builtin_init(lenv* e, lval* a) {
	// check for errors
	LASSERT(a, (a->count == 1), "Function 'head' passed too many arguments!")

	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'head' passed incorrect types!")
	LASSERT(a, (h->count != 0), "Function 'head' passed {}!")

	//take first
	h = lval_take(a, 0);

	// delete first element and return
	lval_del(lval_pop(h, h->count - 1));

	return h;
}

lval* builtin_list(lenv* e, lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lenv* e, lval* a) {
	LASSERT(a, (a->count == 1), "Function 'eval' passed too many parameters!")
	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!")

	h = lval_take(a, 0);
	h->type = LVAL_SEXPR;
	return lval_eval(e, h);
}

lval* lval_join(lval* x, lval* y){
	while(y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

lval* builtin_join(lenv* e, lval* a) {
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

lval* builtin_cons(lenv* e, lval* a) {
	LASSERT(a, (a->count == 2), "Function 'cons' passed insufficient number of parameters!")
	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_NUM), "Function 'cons' passed invalid type")
	h = a->cell[1];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'cons' passed invalid type")

	lval* x = lval_pop(a, 0);
	lval* list = lval_qexpr();
	lval_add(list, x);

	lval* y = lval_join(list, lval_pop(a, 1));
	lval_del(a);
	return y;
}

lval* builtin_len(lenv* e, lval* a) {
	LASSERT(a, (a->count == 1), "Function 'eval' passed too many parameters!")
	lval* h = a->cell[0];
	LASSERT(a, (h->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!")

	h = lval_take(a, 0);
	int len = h->count;
	// delete all non-head elements
	while(h->count > 0) {
		lval_del(lval_pop(h, 0));
	}

	return lval_num(len);
}

lval* builtin_op(lenv* e, lval* a, char* op) {

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

lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
	lval* k = lval_sym(name);
	lval* v = lval_fun(func);
	lenv_put(e, k, v);
	lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e){
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);

	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "join", builtin_join);
	lenv_add_builtin(e, "len", builtin_len);
	lenv_add_builtin(e, "cons", builtin_cons);
	lenv_add_builtin(e, "init", builtin_init);
	lenv_add_builtin(e, "eval", builtin_eval);
}

lval* lval_eval_sexpr(lenv* e, lval* v) {

    // evaluate children
    for(int i = 0; i < v->count; i++) { v->cell[i] = lval_eval(e, v->cell[i]); }

    // error check
    for(int i = 0; i < v->count; i++) {
    	lval* o = v->cell[i];
        if(o->type == LVAL_ERR) { return lval_take(v, i); }
    }

    // empty expression
    if (v->count == 0) { return v; }

    // single expression
    if (v->count == 1) { return lval_take(v, 0); }

    // ensure first elem is function
    lval* s = lval_pop(v, 0);
    if (s->type != LVAL_FUN) {
        lval_del(s);
        lval_del(v);
        return lval_err("expression does not start with a symbol");
    }

    lval* res = s->fun(e, v);
    lval_del(s);
    return res;
}

lval* lval_eval(lenv* e, lval *v){
	if(v->type == LVAL_SYM) {
		lval* x = lenv_get(e, v);
		lval_del(v);
		return x;
	}

    if(v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    return v;
}
