#include "lval.h"
#include <stdio.h>
#include <stdarg.h>

/* Create a new number type lval */
lval* lval_num(long x) {
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->hash = hmap_int_h(x);
  v->as.num = x;
  return v;
}

lval* lval_str(char* s) {
	  lval* v = (lval*)malloc(sizeof(lval));
	  v->type = LVAL_STR;
	  v->hash = hmap_str_h(s);
	  v->as.str = (char*)malloc(strlen(s) + 1);
	  strcpy(v->as.str, s);
	  return v;
}

lval* lval_fun(lbuiltin func) {
	  lval* v = (lval*)malloc(sizeof(lval));
	  v->type = LVAL_FUN;
	  v->hash = hmap_int_h((int*)func);
	  v->as.fun.builtin = func;
	  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
	lval* v = (lval*)malloc(sizeof(lval));

	v->type = LVAL_FUN;
	v->as.fun.builtin = NULL;
	v->as.fun.env = lenv_new();
	v->as.fun.formals = formals;
	v->as.fun.body = body;

	int h = formals->hash ^ body->hash;
	v->hash = h;

	return v;
}

/* Create a new error type lval */
lval* lval_err(char* fmt, ...) {
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_ERR;

  // create a va_list
  va_list va;
  va_start(va, fmt);

  v->as.err = (char*)malloc(512);

  vsnprintf(v->as.err, 511, fmt, va);

  v->as.err = realloc(v->as.err, strlen(v->as.err) + 1);
  v->hash = hmap_str_h(v->as.err);
  va_end(va);

  return v;
}

lenv* lenv_copy(lenv* env) {
	lenv* n = (lenv*)malloc(sizeof(lenv));
	n->par = env->par;
	n->map = hmap_new();
	for(int i = 0; i < env->map->cap; i++) {
		hslot s = env->map->slots[i];
		if(s.used == 1) {
			lval* cp = lval_copy(s.val);
			hmap_put(n->map, cp->hash, cp);
		}
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
	case LVAL_STR: return "String";
	default: return "Unknown";
	}
}

lval* lval_sym(char* s){
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->hash = hmap_str_h(s);
  v->as.sym = (char*)malloc(strlen(s) + 1);
  strcpy(v->as.sym, s);
  return v;
}

lval* lval_sexpr(void){
  lval* v = (lval*)malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->hash = hmap_list_h(0, NULL);
  v->as.list.count = 0;
  v->as.list.cell = NULL;

  return v;
}

lval* lval_qexpr(void) {
	lval* v = (lval*)malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->hash = hmap_list_h(0, NULL);
	v->as.list.count = 0;
	v->as.list.cell = NULL;

	return v;
}

lval* lval_add(lval* e, lval* x) {
  e->as.list.count++;
  e->as.list.cell = realloc(e->as.list.cell, sizeof(lval*) * e->as.list.count);
  e->as.list.cell[e->as.list.count-1] = x;
  e->hash = hmap_list_h(e->as.list.count, e->as.list.cell);
  return e;
}

void lval_del(lval* v){
  switch(v->type){
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

  free(v);
}

void lval_print_str(lval* v) {
	char * escaped = malloc(strlen(v->as.str) + 1);
	strcpy(escaped, v->as.str);
	escaped = mpcf_escape(escaped);

	printf("\"%s\"", escaped);

	free(escaped);
}

/* Print an "lval" */
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%li", v->as.num); break;
    case LVAL_STR: lval_print_str(v); break;
    case LVAL_SYM: printf("%s", v->as.sym); break;
    case LVAL_FUN: if(v->as.fun.builtin) {
			printf("<builtin>");
		} else {
			printf("(\\");
			lval_print(v->as.fun.formals);
			putchar(' ');
			lval_print(v->as.fun.body);
			putchar(')');
		}
		break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_ERR: printf("Error: %s", v->as.err); break;
  }
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);

  for(int i = 0; i < v->as.list.count; i++) {
    lval_print(v->as.list.cell[i]);
    if(i != (v->as.list.count-1)){
        putchar(' ');
    }
  }

  putchar(close);
}

/* Print an "lval" followed by a newline */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_pop(lval* v, int i) {
    lval* x = v->as.list.cell[i];

    // shift memory
    memmove(&v->as.list.cell[i], &v->as.list.cell[i+1], sizeof(lval*) * (v->as.list.count - 1));
    v->as.list.count--;
    v->as.list.cell = realloc(v->as.list.cell, sizeof(lval*) * v->as.list.count);

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
	x->hash = v->hash;

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
				x->as.fun.formals = lval_copy(v->as.fun.formals);
				x->as.fun.body = lval_copy(v->as.fun.body);
			}
			break;
		case LVAL_ERR: x->as.err = (char*)malloc(strlen(v->as.err)+1); strcpy(x->as.err, v->as.err); break;
		case LVAL_SYM: x->as.sym = (char*)malloc(strlen(v->as.sym)+1); strcpy(x->as.sym, v->as.sym); break;

		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->as.list.count = v->as.list.count;
			x->as.list.cell = (lval**)malloc(sizeof(lval*) * x->as.list.count);
			for(int i = 0; i < x->as.list.count; i++) {
				x->as.list.cell[i] = lval_copy(v->as.list.cell[i]);
			}
			break;
	}

	return x;
}

lenv* lenv_new(void) {
	lenv* e = (lenv*)malloc(sizeof(lenv));
	e->par = NULL;
	e->map = hmap_new();
	return e;
}

void lenv_del(lenv* e) {
	for(int i = 0; i < e->map->cap; i++) {
		hslot s = e->map->slots[i];
		if(s.used) { lval_del(s.val); }
	}

	hmap_del(e->map);
	free(e);
}

void lenv_print(lenv* e) {
	for(int i = 0; i < e->map->cap; i++) {
		hslot s = e->map->slots[i];
		if(s.used == 1) {
			puts("{");

			printf("%ui, ", s.hash);
			lval_print(s.val);

			puts("}\n");
		}
	}
}

lval* lenv_get(lenv* e, lval* k) {
	lval* val= hmap_get(e->map, k->hash);
	if(val) {
		return lval_copy(val);
	}

	if(e->par) {
		return lenv_get(e->par, k);
	} else {
		return lval_err("Unbound symbol '%s'!", k->as.sym);
	}
}

void lenv_put(lenv* env, lval* key, lval* val) {
	hmap_put(env->map, key->hash, lval_copy(val));
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
	lval* formals = a->as.list.cell[0];
	for(int i = 0; i < formals->as.list.count; i++) {
		LASSERT(a, (formals->as.list.cell[i]->type == LVAL_SYM),
			"Cannot define non-symbol. Got %s, expected %s.",
			ltype_name(formals->as.list.cell[i]->type), ltype_name(LVAL_SYM));
	}

	formals = lval_pop(a, 0);
	lval* body = lval_pop(a, 0);
	lval_del(a);

	return lval_lambda(formals, body);
}

lval* builtin_head(lenv* e, lval* a) {
	// check error
	LASSERT_NUM("head", a, 1);
	LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("head", a, 0);

	lval* h = lval_take(a, 0);

	// delete all non-head elements
	while(h->as.list.count > 1) {
		lval_del(lval_pop(h, 1));
	}
	return h;
}

lval* builtin_tail(lenv* e, lval* a) {
	// check for errors
	LASSERT_NUM("tail", a, 1);

	lval* h = a->as.list.cell[0];
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

	lval* h = a->as.list.cell[0];
	LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("init", a, 0);

	//take first
	h = lval_take(a, 0);

	// delete first element and return
	lval_del(lval_pop(h, h->as.list.count - 1));

	return h;
}

lval* builtin_list(lenv* e, lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lenv* e, lval* a) {
	LASSERT_NUM("eval", a, 1);
	lval* h = a->as.list.cell[0];
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

	h = lval_take(a, 0);
	h->type = LVAL_SEXPR;
	return lval_eval(e, h);
}

lval* lval_call(lenv* e, lval* f, lval* a) {
	// if builtin, use straight call
	if(f->as.fun.builtin) { return f->as.fun.builtin(e, a); }

	int given = a->as.list.count;
	int total = f->as.fun.formals->as.list.count;

	// there are still arguments to be processed
	while(a->as.list.count) {
		// too many args provided
		if(f->as.fun.formals->as.list.count == 0) {
			lval_del(a);
			return lval_err("Function passed too many arguments. Got %i, expected %i.", given, total);
		}

		lval* sym = lval_pop(f->as.fun.formals, 0);

		// special case - use '&' to deal with variable length arguments
		if(strcmp(sym->as.sym, "&") == 0) {
			// & should always be followed by exactly one another symbol
			if(f->as.fun.formals->as.list.count != 1) {
				lval_del(a);
				return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
			}

			// bind next formal to remaining arguments
			lval* nsym = lval_pop(f->as.fun.formals, 0);
			lenv_put(f->as.fun.env, nsym, builtin_list(e, a));
			lval_del(sym);
			lval_del(nsym);
			break;
		}

		lval* val = lval_pop(a, 0);

		lenv_put(f->as.fun.env, sym, val);

		lval_del(sym);
		lval_del(val);
	}
	lval_del(a);

	// if & remains in formal list it should be bound to empty list
	if(f->as.fun.formals->as.list.count > 0 && strcmp(f->as.fun.formals->as.list.cell[0]->as.sym, "&") == 0) {
		if(f->as.fun.formals->as.list.count != 2) {
			return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
		}

		lval_del(lval_pop(f->as.fun.formals, 0)); 		// forget '&' symbol
		lval* sym = lval_pop(f->as.fun.formals, 0);
		lval* val = lval_qexpr();

		lenv_put(f->as.fun.env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	// check if all formals from lambda has been evaluated
	if(f->as.fun.formals->as.list.count == 0) {
		// execute lambda function and return result
		f->as.fun.env->par = e;
		return builtin_eval(f->as.fun.env, lval_add(lval_sexpr(), lval_copy(f->as.fun.body)));
	} else {
		// return partially evaluated lambda function
		return lval_copy(f);
	}
}

lval* lval_join(lval* x, lval* y){
	while(y->as.list.count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

lval* builtin_ord(lenv* e, lval* a, char* op) {
	LASSERT_NUM(op, a, 2);
	LASSERT_TYPE(op, a, 0, LVAL_NUM);
	LASSERT_TYPE(op, a, 1, LVAL_NUM);

	int r;

	if(strcmp(op, ">") == 0) { r = (a->as.list.cell[0]->as.num > a->as.list.cell[1]->as.num); }
	else if(strcmp(op, "<") == 0) { r = (a->as.list.cell[0]->as.num < a->as.list.cell[1]->as.num); }
	else if(strcmp(op, ">=") == 0) { r = (a->as.list.cell[0]->as.num >= a->as.list.cell[1]->as.num); }
	else if(strcmp(op, "<=") == 0) { r = (a->as.list.cell[0]->as.num <= a->as.list.cell[1]->as.num); }

	lval_del(a);
	return lval_num(r);
}

lval* builtin_lt(lenv* e, lval* a) { return builtin_ord(e, a, "<"); }
lval* builtin_le(lenv* e, lval* a) { return builtin_ord(e, a, "<="); }
lval* builtin_gt(lenv* e, lval* a) { return builtin_ord(e, a, ">"); }
lval* builtin_ge(lenv* e, lval* a) { return builtin_ord(e, a, ">="); }

int lval_eq(lval* x, lval* y) {
	// different types are always unequal
	if(x->type != y->type) { return 0; }

	switch(x->type) {
		case LVAL_NUM: return (x->as.num == y->as.num);
		case LVAL_STR: return (strcmp(x->as.str, y->as.str) == 0);
		case LVAL_SYM: return (strcmp(x->as.sym, y->as.sym) == 0);
		case LVAL_ERR: return (strcmp(x->as.err, y->as.err) == 0);
		case LVAL_FUN:
			if(x->as.fun.builtin) {
				return (x->as.fun.builtin == y->as.fun.builtin);
			} else {
				return lval_eq(x->as.fun.formals, y->as.fun.formals) && lval_eq(x->as.fun.body, y->as.fun.body);
			}
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			if(x->as.list.count != y->as.list.count) { return 0; }
			for(int i = 0; i < x->as.list.count; i++) {
				if(!lval_eq(x->as.list.cell[i], y->as.list.cell[i])) { return 0; }
			}
			return 1;
		break;
	}

	return 0;
}

lval* builtin_bool(lenv* e, lval* a, char* op) {
	LASSERT_NUM(op, a, 2);
	LASSERT_TYPE(op, a, 0, LVAL_NUM);
	LASSERT_TYPE(op, a, 1, LVAL_NUM);

	if(strcmp(op, "&&") == 0) {
		lval* x = lval_pop(a, 0);
		if(x->as.num != 0){
			lval* y = lval_pop(a, 0);
			lval_del(a);
			lval_del(x);
			return y;
		} else {
			lval_del(a);
			return x;
		}
	}
	else if(strcmp(op, "||") == 0) {
		lval* x = lval_pop(a, 0);
		if(x->as.num == 0){
			lval* y = lval_pop(a, 0);
			lval_del(a);
			lval_del(x);
			return y;
		} else {
			lval_del(a);
			return x;
		}
	}

	return lval_err("Unsupported logical operator '%s'.", op);
}

lval* builtin_and(lenv* e, lval* a) { return builtin_bool(e, a, "&&"); }
lval* builtin_or(lenv* e, lval* a) { return builtin_bool(e, a, "||"); }
lval* builtin_neq(lenv* e, lval* a) {
	LASSERT_NUM("!", a, 1);
	LASSERT_TYPE("!", a, 0, LVAL_NUM);

	int x = a->as.list.cell[0]->as.num == 0 ? 1 : 0;
	lval_del(a);

	return lval_num(x);
}

lval* lval_cmp(lenv* e, lval* a, char* op) {
	LASSERT_NUM(op, a, 2);

	int r;

	if(strcmp(op, "==") == 0) { r = lval_eq(a->as.list.cell[0], a->as.list.cell[1]); }
	else if(strcmp(op, "!=") == 0) { r = !lval_eq(a->as.list.cell[0], a->as.list.cell[1]); }

	lval_del(a);
	return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a) { return lval_cmp(e, a, "=="); }
lval* builtin_ne(lenv* e, lval* a) { return lval_cmp(e, a, "!="); }

lval* builtin_join(lenv* e, lval* a) {
	for(int i = 0; i < a->as.list.count; i++) {
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval* x = lval_pop(a, 0);

	while(a->as.list.count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);
	return x;
}

lval* builtin_if(lval* e, lval* a) {
	LASSERT_NUM("if", a, 3);
	LASSERT_TYPE("if", a, 0, LVAL_NUM);
	LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
	LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

	// mark both expressions as evaluable
	lval* x;
	a->as.list.cell[1]->type = LVAL_SEXPR;
	a->as.list.cell[2]->type = LVAL_SEXPR;

	if(a->as.list.cell[0]->as.num) {
		x = lval_eval(e, lval_pop(a, 1));
	} else {
		x = lval_eval(e, lval_pop(a, 2));
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
	int len = h->as.list.count;
	// delete all non-head elements
	while(h->as.list.count > 0) {
		lval_del(lval_pop(h, 0));
	}

	return lval_num(len);
}

lval* builtin_var(lenv* e, lval* a, char* op) {
	LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

	lval* syms = a->as.list.cell[0];

	for(int i = 0; i < syms->as.list.count; i++) {
		LASSERT(a, (syms->as.list.cell[i]->type == LVAL_SYM),
			"Function 'def' cannot define non-symbol! Get %s, expected %s.",
			ltype_name(syms->as.list.cell[i]->type), ltype_name(LVAL_SYM));
	}

	LASSERT(a, (syms->as.list.count == a->as.list.count-1),
			"Function 'def' passed too many arguments for symbols. Got %i, expected %i.",
			syms->as.list.count, a->as.list.count-1);

	// assign copies of values to symbols
	for(int i = 0; i < syms->as.list.count; i++){
		if (strcmp(op, "def") == 0) { lenv_def(e, syms->as.list.cell[i], a->as.list.cell[i+1]); }
		if (strcmp(op, "=") == 0) { lenv_put(e, syms->as.list.cell[i], a->as.list.cell[i+1]); }
	}

	lval_del(a);
	return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "="); }

lval* builtin_op(lenv* e, lval* a, char* op) {
    // ensure all arguments are numbers
    for(int i = 0; i < a->as.list.count; i++) { LASSERT_TYPE(op, a, i, LVAL_NUM); }

    lval* x = lval_pop(a, 0);

    // try to perform unary negation
    if ((strcmp(op, "-") == 0) && a->as.list.count == 0) { x->as.num = -x->as.num; }

    // for all elements
    while(a->as.list.count > 0) {
        lval* y = lval_pop(a, 0);

        // perform operation
        if (strcmp(op, "+") == 0) { x->as.num += y->as.num; }
        if (strcmp(op, "-") == 0) { x->as.num -= y->as.num; }
        if (strcmp(op, "*") == 0) { x->as.num *= y->as.num; }
        if (strcmp(op, "/") == 0) {
            if(y->as.num == 0) {
                lval_del(x);
                lval_del(y);
                lval_del(a);
                x = lval_err("Division by zero!"); break;
            } else {
                x->as.num /= y->as.num;
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

lval* builtin_print(lenv* e, lval* a) {
	for(int i = 0; i < a->as.list.count; i++) {
		lval_print(a->as.list.cell[i]);
		putchar(' ');
	}

	putchar('\n');
	lval_del(a);

	return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
	LASSERT_NUM("error", a, 1);
	LASSERT_TYPE("error", a, 0, LVAL_STR);

	lval* err = lval_err(a->as.list.cell[0]->as.str);

	lval_del(a);
	return err;
}

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

	lenv_add_builtin(e, "if", builtin_if);
	lenv_add_builtin(e, "==", builtin_eq);
	lenv_add_builtin(e, "!=", builtin_ne);
	lenv_add_builtin(e, ">", builtin_gt);
	lenv_add_builtin(e, ">=", builtin_ge);
	lenv_add_builtin(e, "<", builtin_lt);
	lenv_add_builtin(e, "<=", builtin_le);
	lenv_add_builtin(e, "||", builtin_or);
	lenv_add_builtin(e, "&&", builtin_and);
	lenv_add_builtin(e, "!", builtin_neq);

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
	lenv_add_builtin(e, "print", builtin_print);
	lenv_add_builtin(e, "error", builtin_error);
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
    for(int i = 0; i < v->as.list.count; i++) { v->as.list.cell[i] = lval_eval(e, v->as.list.cell[i]); }
    for(int i = 0; i < v->as.list.count; i++) { lval* o = v->as.list.cell[i]; if(o->type == LVAL_ERR) { return lval_take(v, i); } }

    // empty expression
    if (v->as.list.count == 0) { return v; }

    // single expression
    if (v->as.list.count == 1) { return lval_take(v, 0); }

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

unsigned int hmap_list_h(int n, lval* l) {
	int hash = 31;
	for (int i = 0; i < n; i++) {
		hash = 31*hash + l[i].hash;
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

int hmap_put(hmap* h, int hash, lval* val) {
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

lval* hmap_get(hmap* h, int hash) {
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
