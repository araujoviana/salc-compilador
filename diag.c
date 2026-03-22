// Eu não sei se é exatamente isso q esse arquivo faz
// pq o moodle ta bugado

#include "opt.h"
#include <stdio.h>

// Monta a mensagem de erro, usada pelo lexer e parser
// para centralizar mensagens de erro
int diag_error(char *token_expected, char *token_found, int line) {
  fprintf(stderr, "%d# Error: Expected %s, Found %s\n", line, token_expected,
          token_found);
  return 0;
}

int diag_info(char *msg) {

  // Imprime info se flag trace está ativa
  if (!opts_get(OPT_TRACE))
    return 0;

  printf("%s\n", msg);
  return 0;
}
