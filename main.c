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

lval eval_op(lval x, char* op, lval y) {
  
  /* If either value is an error return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }
  
  /* Otherwise do maths on the number values */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero return error instead of result */
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  
  if (strstr(t->tag, "number")) {
    /* Check if there is some error in conversion */
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }
  
  char* op = t->children[1]->contents;  
  lval x = eval(t->children[2]);
  
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  
  return x;  
}

int main(int argc, char** argv) {
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Qsp = mpc_new("Qsp");
  
  mpca_lang(MPC_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      Qsp    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Qsp);
  
  puts("Qsp Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("qsp> ");
    add_history(input);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Qsp, &r)) {
      
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
      
    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
    
  }
  
  mpc_cleanup(4, Number, Operator, Expr, Qsp);
  
  return 0;
}