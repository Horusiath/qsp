#include "mpc.h"
#include "lval.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs("qsp> ", stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else

#include <editline/readline.h>
#include <editline/history.h>

#endif

mpc_parser_t* Number;
mpc_parser_t* String;
mpc_parser_t* Symbol;
mpc_parser_t* Comment;
mpc_parser_t* Qexpr;
mpc_parser_t* Sexpr;
mpc_parser_t* Expr;
mpc_parser_t* Qsp;

mem_heap* HEAP;

lval* lval_read_num(mpc_ast_t* t) {
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read_str(mpc_ast_t* t) {
	t->contents[strlen(t->contents) - 1] = '\0'; // cut off final '"' character
	char* unesc = (char*)malloc(strlen(t->contents)+1);
	strcpy(unesc, t->contents+1);

	unesc = mpcf_unescape(unesc);

	lval* str = lval_str(unesc);

	free(unesc);
	return str;
}

lval* lval_read(mpc_ast_t* t){
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  if (strstr(t->tag, "string")) { return lval_read_str(t); }

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

  for (int i = 0; i < t->children_num; ++i)
  {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "comment") == 0) { continue; }

    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval* builtin_load(lenv* e, lval* a) {
	LASSERT_NUM("load", a, 1);
	LASSERT_TYPE("load", a, 0, LVAL_STR);

	// parse file given by string name
	mpc_result_t r;
	if(mpc_parse_contents(a->as.list.cell[0]->as.str, Qsp, &r)) {
		// read contents
		lval* expr = lval_read(r.output);
		mpc_ast_delete(r.output);

		while(expr->as.list.count) {
			lval* x = lval_eval(e, lval_pop(expr, 0));
			if(x->type == LVAL_ERR) { lval_print(x); }
			lval_del(x);
		}
		// delete expressions and arguments
		lval_del(expr);
		lval_del(a);
		return lval_sexpr();
	} else {
		// get parse error
		char* err_msg = mpc_err_string(r.error);
		mpc_err_delete(r.error);

		lval* err = lval_err("Could not load library %s", err_msg);
		free(err_msg);
		lval_del(a);

		return err;
	}
}

int main(int argc, char** argv) {
  Number 	= mpc_new("number");
  String 	= mpc_new("string");
  Symbol 	= mpc_new("symbol");
  Comment 	= mpc_new("comment");
  Qexpr 	= mpc_new("qexpr");
  Sexpr 	= mpc_new("sexpr");
  Expr 		= mpc_new("expr");
  Qsp 		= mpc_new("qsp");

  mpca_lang(MPC_LANG_DEFAULT,
		  "                                              \
		    number  : /-?[0-9]+/ ;                       \
		    symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
		    string  : /\"(\\\\.|[^\"])*\"/ ;             \
		    comment : /;[^\\r\\n]*/ ;                    \
		    sexpr   : '(' <expr>* ')' ;                  \
		    qexpr   : '{' <expr>* '}' ;                  \
		    expr    : <number>  | <symbol> | <string>    \
		            | <comment> | <sexpr>  | <qexpr>;    \
		    qsp  : /^/ <expr>* /$/ ;                     \
		  ",
    Number, String, Comment, Symbol, Sexpr, Qexpr, Expr, Qsp);

  puts("Qsp Version 0.0.3.0");
  puts("Press Ctrl+c to Exit\n");
  
  HEAP = heap_new();
  lenv* e = lenv_new();
  lenv_add_builtins(e);
  lenv_add_builtin(e, "load", builtin_load);

  if(argc >= 2) {
	  for(int i = 1; i < argc; i++) {
		  // create argument list with a single argument being filename
		  lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
		  // pass to builtin load and get the result
		  lval* x = builtin_load(e, args);

		  if(x->type == LVAL_ERR) { lval_println(x); }

		  lval_del(x);
	  }
  }

  while (1) {
  
    char* input = readline("qsp> ");
    add_history(input);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Qsp, &r)) {
      lval* x = lval_eval(e, lval_read(r.output));
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
      
    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
  }
  lenv_del(e);
  heap_del(HEAP);
  
  mpc_cleanup(8, Number, String, Comment, Symbol, Sexpr, Qexpr, Expr, Qsp);
  
  return 0;
}
