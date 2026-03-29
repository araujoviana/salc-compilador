#include "diag.h"
#include "log.h"

#include <stdio.h>

int diag_error(const char *token_expected, const char *token_found, int line) {
  fprintf(stderr, "%d# Error: Expected %s, Found %s\n", line, token_expected,
          token_found);
  return 0;
}

int diag_info(const char *msg) {
  return log_trace(msg);
}
