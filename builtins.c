#include "lval.h"
#include "hmap.h"
#include <stdio.h>
#include <stdarg.h>


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
	lval* head = lval_cp(h->as.list.cell[0]);

	// create new Q-Expression with head of previous one as only element
	lval* q = lval_qexpr();
	lval_add(q, head);
	return q;
}

lval* builtin_tail(lenv* e, lval* a) {
	// check for errors
	LASSERT_NUM("tail", a, 1);

	lval* h = a->as.list.cell[0];
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", a, 0);

	//take first
	h = lval_take(a, 0);

	// copy all elements except first to new Q-Expression
	lval* q = lval_qexpr();
	for(int i=1; i < h->as.list.count; i++) {
		lval_add(q, lval_cp(h->as.list.cell[i]));
	}

	return q;
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
	LASSERT_TYPE(op, a, 0, LVAL_QEXPR);

	lval* syms = a->as.list.cell[0];

	for(int i = 0; i < syms->as.list.count; i++) {
		LASSERT(a, (syms->as.list.cell[i]->type == LVAL_SYM),
			"Function '%s' cannot define non-symbol! Get %s, expected %s.",
			op, ltype_name(syms->as.list.cell[i]->type), ltype_name(LVAL_SYM));
	}

	LASSERT(a, (syms->as.list.count == a->as.list.count-1),
			"Function '%s' passed too many arguments for symbols. Got %i, expected %i.",
			op, syms->as.list.count, a->as.list.count-1);

	// assign copies of values to symbols
	for(int i = 0; i < syms->as.list.count; i++){
		if (strcmp(op, "def") == 0) { lenv_def(e, syms->as.list.cell[i], a->as.list.cell[i+1]); }
		else if (strcmp(op, "=") == 0) { lenv_put(e, syms->as.list.cell[i], a->as.list.cell[i+1]); }
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

