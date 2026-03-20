#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static Token token;
static FILE *src;
static int line_cnt = 1;

static void advance(void) { token = lex_next(src, &line_cnt); }

static bool accept(Category c) {
  if (token.category == c) {
    advance();
    return true;
  }
  return false;
}

static void expect(Category c, const char *expected) {
  if (!accept(c)) {
    diag_error((char *)expected, token.lexema, token.line);
    exit(EXIT_FAILURE);
  }
}

static void parse_bco(void) {
  expect(sSTART, "start");
  while (starts_cmd(token.category)) {
    parse_cmd();
    expect(sPTO_VIRG, ";");
  }

  expect(sEND, "end");
}

static void parse_if(void) {
  expect(sIF, "if");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_cmd();
  if (accept(sELSE))
    parse_cmd();
}
