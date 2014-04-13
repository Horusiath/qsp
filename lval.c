#include "lval.h"
#include <stdio.h>
#include <stdarg.h>

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
	  v->builtin = func;
	  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
	lval* v = (lval*)malloc(sizeof(lval));

	v->type = LVAL_FUN;
	v->builtin = NULL;
	v->env = lenv_new();
	v->formals = formals;
	v->body = body;

	return v;
}

/* Create a new error type lval */
lval* lval_err(char* fmt, ...) {
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_ERR;

  // create a va_list
  va_list va;
  va_start(va, fmt);

  v->err = (char*)malloc(512);

  vsnprintf(v->err, 511, fmt, va);

  v->err = realloc(v->err, strlen(v->err) + 1);
  va_end(va);

  return v;
}

lenv* lenv_copy(lenv* env) {
	lenv* n = (lenv*)malloc(sizeof(lenv));
	n->par = env->par;
	n->count = env->count;
	n->syms = (char**)malloc(sizeof(char*) * n->count);
	n->vals = (lval**)malloc(sizeof(lval*) * n->count);
	for(int i = 0; i < n->count; i++) {
		n->syms[i] = (char*)malloc(strlen(env->syms[i]) + 1);
		strcpy(n->syms[i], env->syms[i]);
		n->vals[i] = lval_copy(env->vals[i]);
	}
	return n;
}

char * ltype_name(int t) {
	switch(t) {
	case LVAL_ERR: return "Error";
	case LVAL_FUN: return "Function";
	case LVAL_NUM: return "Number";
	case LVAL_QEXPR: return "Q-Expression";
	case LVAL_SEXPR: return "S-Expression";
	case LVAL_SYM: return "Symbol";
	default: return "Unknown";
	}
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
    case LVAL_FUN:
    	if(!v->builtin){
    		lenv_del(v->env);
    		lval_del(v->formals);
    		lval_del(v->body);
    	}
    break;
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
    case LVAL_FUN: if(v->builtin) {
			printf("<builtin>");
		} else {
			printf("(\\");
			lval_print(v->formals);
			putchar(' ');
			lval_print(v->body);
			putchar(')');
		}
		break;
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
		case LVAL_FUN:
			if(v->builtin){
				x->builtin = v->builtin;
			} else {
				x->builtin = NULL;
				x->env = lenv_copy(v->env);
				x->formals = lval_copy(v->formals);
				x->body = lval_copy(v->body);
			}
			break;
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
	e->par = NULL;
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

	if(e->par) {
		return lenv_get(e->par, k);
	} else {
		return lval_err("Unbound symbol '%s'!", k->sym);
	}
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

void lenv_def(lenv* e, lval* v, lval* k) {
	while(e->par) { e = e->par; }
	lenv_put(e, v, k);
}

lval* builtin_lambda(lenv* e, lval* a) {
	LASSERT_NUM("\\", a, 2);
	LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
	LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

	// check if first q-expr has only symbols
	lval* formals = a->cell[0];
	for(int i = 0; i < formals->count; i++) {
		LASSERT(a, (formals->cell[i]->type == LVAL_SYM),
			"Cannot define non-symbol. Got %s, expected %s.",
			ltype_name(formals->cell[i]->type), ltype_name(LVAL_SYM));
	}

	formals = lval_pop(a, 0);
	lval* body = lval_pop(a, 0);
	lval_del(a);

	return lval_lambda(formals, body);
}

lval* builtin_head(lenv* e, lval* a) {
	// check error
	LASSERT_NUM("head", a, 1);

	lval* h = a->cell[0];
	LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("head", a, 0);

	h = lval_take(a, 0);

	// delete all non-head elements
	while(h->count > 1) {
		lval_del(lval_pop(h, 1));
	}
	return h;
}

lval* builtin_tail(lenv* e, lval* a) {
	// check for errors
	LASSERT_NUM("tail", a, 1);

	lval* h = a->cell[0];
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", a, 0);

	//take first
	h = lval_take(a, 0);

	// delete first element and return
	lval_del(lval_pop(h, 0));

	return h;
}

lval* builtin_init(lenv* e, lval* a) {
	// check for errors
	LASSERT_NUM("init", a, 1);

	lval* h = a->cell[0];
	LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("init", a, 0);

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
	LASSERT_NUM("eval", a, 1);
	lval* h = a->cell[0];
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

	h = lval_take(a, 0);
	h->type = LVAL_SEXPR;
	return lval_eval(e, h);
}

lval* lval_call(lenv* e, lval* f, lval* a) {
	// if builtin, use straight call
	if(f->builtin) { return f->builtin(e, a); }

	int given = a->count;
	int total = f->formals->count;

	// there are still arguments to be processed
	while(a->count) {
		// too many args provided
		if(f->formals->count == 0) {
			lval_del(a);
			return lval_err("Function passed too many arguments. Got %i, expected %i.", given, total);
		}

		lval* sym = lval_pop(f->formals, 0);

		// special case - use '&' to deal with variable length arguments
		if(strcmp(sym->sym, "&") == 0) {
			// & should always be followed by exactly one another symbol
			if(f->formals->count != 1) {
				lval_del(a);
				return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
			}

			// bind next formal to remaining arguments
			lval* nsym = lval_pop(f->formals, 0);
			lenv_put(f->env, nsym, builtin_list(e, a));
			lval_del(sym);
			lval_del(nsym);
			break;
		}

		lval* val = lval_pop(a, 0);

		lenv_put(f->env, sym, val);

		lval_del(sym);
		lval_del(val);
	}
	lval_del(a);

	// if & remains in formal list it should be bound to empty list
	if(f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
		if(f->formals->count != 2) {
			return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
		}

		lval_del(lval_pop(f->formals, 0)); 		// forget '&' symbol
		lval* sym = lval_pop(f->formals, 0);
		lval* val = lval_qexpr();

		lenv_put(f->env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	// check if all formals from lambda has been evaluated
	if(f->formals->count == 0) {
		// execute lambda function and return result
		f->env->par = e;
		return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
	} else {
		// return partially evaluated lambda function
		return lval_copy(f);
	}
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
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval* x = lval_pop(a, 0);

	while(a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);
	return x;
}

lval* builtin_cons(lenv* e, lval* a) {
	LASSERT_NUM("cons", a, 2);
	LASSERT_TYPE("cons", a, 0, LVAL_NUM);
	LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

	lval* x = lval_pop(a, 0);
	lval* list = lval_qexpr();
	lval_add(list, x);

	lval* y = lval_join(list, lval_pop(a, 1));
	lval_del(a);
	return y;
}

lval* builtin_len(lenv* e, lval* a) {
	LASSERT_NUM("len", a, 1);
	LASSERT_TYPE("len", a, 0, LVAL_QEXPR);

	lval* h = lval_take(a, 0);
	int len = h->count;
	// delete all non-head elements
	while(h->count > 0) {
		lval_del(lval_pop(h, 0));
	}

	return lval_num(len);
}

lval* builtin_var(lenv* e, lval* a, char* op) {
	LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

	lval* syms = a->cell[0];

	for(int i = 0; i < syms->count; i++) {
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
			"Function 'def' cannot define non-symbol! Get %s, expected %s.",
			ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
	}

	LASSERT(a, (syms->count == a->count-1),
			"Function 'def' passed too many arguments for symbols. Got %i, expected %i.",
			syms->count, a->count-1);

	// assign copies of values to symbols
	for(int i = 0; i < syms->count; i++){
		if (strcmp(op, "def") == 0) { lenv_def(e, syms->cell[i], a->cell[i+1]); }
		if (strcmp(op, "=") == 0) { lenv_put(e, syms->cell[i], a->cell[i+1]); }
	}

	lval_del(a);
	return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "="); }

lval* builtin_op(lenv* e, lval* a, char* op) {
    // ensure all arguments are numbers
    for(int i = 0; i < a->count; i++) { LASSERT_TYPE(op, a, i, LVAL_NUM); }

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

	lenv_add_builtin(e, "\\", builtin_lambda);
	lenv_add_builtin(e, "def", builtin_def);
	lenv_add_builtin(e, "=", builtin_put);
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
    for(int i = 0; i < v->count; i++) { v->cell[i] = lval_eval(e, v->cell[i]); }
    for(int i = 0; i < v->count; i++) { lval* o = v->cell[i]; if(o->type == LVAL_ERR) { return lval_take(v, i); } }

    // empty expression
    if (v->count == 0) { return v; }

    // single expression
    if (v->count == 1) { return lval_take(v, 0); }

    // ensure first elem is function
    lval* s = lval_pop(v, 0);
    if (s->type != LVAL_FUN) {
        lval* err = lval_err("S-Expression starts with incorrect type! Got %s, expected %s",
        		ltype_name(s->type), ltype_name(LVAL_FUN));
        lval_del(s);
        lval_del(v);

        return err;
    }

    lval* res = lval_call(e, s, v);
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
