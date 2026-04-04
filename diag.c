/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#include "diag.h"
#include "log.h"

#include <stdio.h>

int diag_error(const char *token_expected, const char *token_found, int line) {
  if (token_expected == NULL) {
    token_expected = "token valido";
  }

  if (token_found == NULL) {
    token_found = "(nulo)";
  }

  fprintf(stderr, "Linha %d: esperado %s, encontrado \"%s\".\n", line,
          token_expected, token_found);
  return -1;
}

int diag_info(const char *msg) { return log_trace(msg); }
