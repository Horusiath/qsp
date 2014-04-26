#include "lval.h"
#include "hmap.h"
#include <stdio.h>
#include <stdarg.h>


char * ltype_name(int t) {
	switch(t) {
	case LVAL_UNDEF: return "Undefined";
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

/* Create a new number type lval */
lval* lval_num(long x) {
  lval* v = lval_new();
  v->type = LVAL_NUM;
  v->hash = hmap_int_h(x);
  v->as.num = x;
  return v;
}

lval* lval_str(char* s) {
	  lval* v = lval_new();
	  v->type = LVAL_STR;
	  v->hash = hmap_str_h(s);
	  v->as.str = (char*)malloc(strlen(s) + 1);
	  strcpy(v->as.str, s);
	  return v;
}

lval* lval_fun(lbuiltin func) {
	  lval* v = lval_new();
	  v->type = LVAL_FUN;
	  v->hash = hmap_int_h((int*)func);
	  v->as.fun.builtin = func;
	  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
	lval* v = lval_new();

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
  lval* v = lval_new();
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
			lval* cp = lval_cp(s.val);
			hmap_put(n->map, cp->hash, cp);
		}
	}

	return n;
}

lval* lval_sym(char* s){
  lval* v = lval_new();
  v->type = LVAL_SYM;
  v->hash = hmap_str_h(s);
  v->as.sym = (char*)malloc(strlen(s) + 1);
  strcpy(v->as.sym, s);
  return v;
}

lval* lval_sexpr(void){
  lval* v = lval_new();
  v->type = LVAL_SEXPR;
  v->hash = hmap_list_h(0, NULL);
  v->as.list.count = 0;
  v->as.list.cell = NULL;

  return v;
}

lval* lval_qexpr(void) {
	lval* v = lval_new();
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

void lval_print_str(lval* v) {
	char * escaped = (char*)malloc(strlen(v->as.str) + 1);
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
		return lval_cp(val);
	}

	if(e->par) {
		return lenv_get(e->par, k);
	} else {
		return lval_err("Unbound symbol '%s'!", k->as.sym);
	}
}

void lenv_put(lenv* env, lval* key, lval* val) {
	hmap_put(env->map, key->hash, lval_cp(val));
}

void lenv_def(lenv* e, lval* v, lval* k) {
	while(e->par) { e = e->par; }
	lenv_put(e, v, k);
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
		return builtin_eval(f->as.fun.env, lval_add(lval_sexpr(), lval_cp(f->as.fun.body)));
	} else {
		// return partially evaluated lambda function
		return lval_cp(f);
	}
}

lval* lval_join(lval* x, lval* y){
	while(y->as.list.count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

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

unsigned int hmap_list_h(int n, lval* l) {
	int hash = 31;
	for (int i = 0; i < n; i++) {
		hash = 31*hash + l[i].hash;
	}
	return hash;
}
