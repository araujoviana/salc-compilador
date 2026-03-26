#include "parser.h"
#include "diag.h"
#include "lex.h"
#include "log.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static Token token;
static Token next_token;
static FILE *src;
static int line_cnt = 1;

static void parse_ini(void);
static void parse_glob(void);
static void parse_subs(void);
static void parse_decls(void);
static void parse_decl_item(void);
static void parse_tpo(void);
static void parse_func(void);
static void parse_proc(void);
static void parse_princ(void);
static void parse_locals_opt(void);
static void parse_param(void);
static void parse_bco(void);
static void parse_cmd(void);
static void parse_out(void);
static void parse_inp(void);
static void parse_if(void);
static void parse_mat(void);
static void parse_wlist(void);
static void parse_when(void);
static void parse_otherwise(void);
static void parse_wcnd(void);
static void parse_witem(void);
static void parse_wint(void);
static void parse_fr(void);
static void parse_loop(void);
static void parse_ret(void);
static void parse_atr(void);
static void parse_call(void);
static void parse_vec(void);
static void parse_id(void);
static void parse_elem(void);
static void parse_expr(void);
static void parse_exlog(void);
static void parse_exrel(void);
static void parse_exari(void);
static void parse_exarp(void);
static void parse_fact(void);
static void parse_primary(void);
static void parse_litl(void);

static void advance(void) {
  token = next_token;
  next_token = lex_next(src, &line_cnt);
}

static bool accept(Category c) {
  if (token.category == c) {
    advance();
    return true;
  }
  return false;
}

static void fail(const char *expected) {
  diag_error(expected, token.lexema, token.line);
  exit(EXIT_FAILURE);
}

static void expect(Category c, const char *expected) {
  if (!accept(c)) {
    fail(expected);
  }
}

static bool starts_cmd(Category c) {
  return c == sPRINT || c == sSCAN || c == sIF || c == sMATCH || c == sFOR ||
         c == sLOOP || c == sRETURN || c == sSTART || c == sIDENTIF;
}

static bool is_relop(Category c) {
  return c == sMAIOR || c == sMAIORIG || c == sIGUAL || c == sMENOR ||
         c == sMENORIG || c == sDIFERENTE;
}

static bool is_bool_type_token(void) {
  return token.category == sBOOL && strcmp(token.lexema, "bool") == 0;
}

static bool is_bool_literal_token(void) {
  return token.category == sBOOL && (strcmp(token.lexema, "true") == 0 ||
                                     strcmp(token.lexema, "false") == 0);
}

static void parse_id(void) { expect(sIDENTIF, "identificador"); }

static void parse_decl_item(void) {
  parse_id();
  if (accept(sABRE_COLCH)) {
    expect(sCTEINT, "constante inteira");
    expect(sFECHA_COLCH, "]");
  }
}

static void parse_decls(void) {
  log_trace("parse_decls");
  parse_decl_item();
  while (accept(sVIRGULA)) {
    parse_decl_item();
  }
  expect(sDOIS_PTOS, ":");
  parse_tpo();
  expect(sPTO_VIRG, ";");
}

static void parse_tpo(void) {
  if (token.category == sINT) {
    advance();
  } else if (is_bool_type_token()) {
    advance();
  } else if (token.category == sCHAR) {
    advance();
  } else {
    fail("tipo (int|bool|char)");
  }

  while (accept(sABRE_COLCH)) {
    expect(sCTEINT, "constante inteira");
    expect(sFECHA_COLCH, "]");
  }
}

static void parse_glob(void) {
  log_trace("parse_glob");
  expect(sGLOBALS, "globals");
  parse_decls();
  while (token.category == sIDENTIF) {
    parse_decls();
  }
}

static void parse_locals_opt(void) {
  if (accept(sLOCALS)) {
    parse_decls();
    while (token.category == sIDENTIF) {
      parse_decls();
    }
  }
}

static void parse_param(void) {
  parse_id();
  expect(sDOIS_PTOS, ":");
  parse_tpo();
  while (accept(sVIRGULA)) {
    parse_id();
    expect(sDOIS_PTOS, ":");
    parse_tpo();
  }
}

static void parse_func(void) {
  log_trace("parse_func");
  expect(sFN, "fn");
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) {
    parse_param();
  }
  expect(sFECHA_PARENT, ")");
  expect(sDOIS_PTOS, ":");
  parse_tpo();
  parse_locals_opt();
  parse_bco();
}

static void parse_proc(void) {
  log_trace("parse_proc");
  expect(sPROC, "proc");
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) {
    parse_param();
  }
  expect(sFECHA_PARENT, ")");
  parse_locals_opt();
  parse_bco();
}

static void parse_subs(void) {
  log_trace("parse_subs");
  while (token.category == sFN ||
         (token.category == sPROC && next_token.category != sMAIN)) {
    if (token.category == sFN) {
      parse_func();
    } else {
      parse_proc();
    }
  }
}

static void parse_princ(void) {
  log_trace("parse_princ");
  expect(sPROC, "proc");
  expect(sMAIN, "main");
  expect(sABRE_PARENT, "(");
  expect(sFECHA_PARENT, ")");
  parse_locals_opt();
  parse_bco();
}

static void parse_ini(void) {
  log_trace("parse_ini");
  expect(sMODULE, "module");
  parse_id();
  expect(sPTO_VIRG, ";");
  if (token.category == sGLOBALS) {
    parse_glob();
  }
  parse_subs();
  parse_princ();
}

static void parse_bco(void) {
  log_trace("parse_bco");
  expect(sSTART, "start");
  while (starts_cmd(token.category)) {
    parse_cmd();
    expect(sPTO_VIRG, ";");
  }
  expect(sEND, "end");
}

static void parse_vec(void) {
  parse_id();
  expect(sABRE_COLCH, "[");
  parse_expr();
  expect(sFECHA_COLCH, "]");
}

static void parse_call(void) {
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category != sFECHA_PARENT) {
    parse_expr();
    while (accept(sVIRGULA)) {
      parse_expr();
    }
  }
  expect(sFECHA_PARENT, ")");
}

static void parse_atr(void) {
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_vec();
  } else {
    parse_id();
  }
  expect(sATRIB, ":=");
  parse_elem();
}

static void parse_out(void) {
  expect(sPRINT, "print");
  expect(sABRE_PARENT, "(");
  parse_elem();
  while (accept(sVIRGULA)) {
    parse_elem();
  }
  expect(sFECHA_PARENT, ")");
}

static void parse_inp(void) {
  expect(sSCAN, "scan");
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_vec();
  } else if (token.category == sIDENTIF) {
    parse_id();
  } else {
    fail("identificador ou vetor");
  }
  expect(sFECHA_PARENT, ")");
}

static void parse_if(void) {
  expect(sIF, "if");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_cmd();
  if (accept(sELSE)) {
    parse_cmd();
  }
}

static void parse_wint(void) {
  accept(sSUBRAT);
  expect(sCTEINT, "constante inteira");
}

static void parse_witem(void) {
  parse_wint();
  if (accept(sPTOPTO)) {
    parse_wint();
  }
}

static void parse_wcnd(void) {
  parse_witem();
  while (accept(sVIRGULA)) {
    parse_witem();
  }
}

static void parse_when(void) {
  expect(sWHEN, "when");
  parse_wcnd();
  expect(sIMPLIC, "=>");
  parse_cmd();
  expect(sPTO_VIRG, ";");
}

static void parse_otherwise(void) {
  expect(sOTHERWISE, "otherwise");
  expect(sIMPLIC, "=>");
  parse_cmd();
  expect(sPTO_VIRG, ";");
}

static void parse_wlist(void) {
  parse_when();
  while (token.category == sWHEN) {
    parse_when();
  }
  if (token.category == sOTHERWISE) {
    parse_otherwise();
  }
}

static void parse_mat(void) {
  expect(sMATCH, "match");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_wlist();
  expect(sEND, "end");
}

static void parse_fr(void) {
  expect(sFOR, "for");
  parse_atr();
  expect(sTO, "to");
  parse_expr();
  if (accept(sSTEP)) {
    if (token.category == sIDENTIF) {
      parse_id();
    } else {
      expect(sCTEINT, "constante inteira");
    }
  }
  expect(sDO, "do");
  parse_cmd();
}

static void parse_loop(void) {
  expect(sLOOP, "loop");
  if (accept(sWHILE)) {
    expect(sABRE_PARENT, "(");
    parse_expr();
    expect(sFECHA_PARENT, ")");
    parse_cmd();
    return;
  }

  while (starts_cmd(token.category)) {
    parse_cmd();
    expect(sPTO_VIRG, ";");
  }
  expect(sUNTIL, "until");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
}

static void parse_ret(void) {
  expect(sRETURN, "ret");
  parse_elem();
}

static void parse_cmd(void) {
  log_trace("parse_cmd");
  switch (token.category) {
  case sPRINT:
    parse_out();
    break;
  case sSCAN:
    parse_inp();
    break;
  case sIF:
    parse_if();
    break;
  case sMATCH:
    parse_mat();
    break;
  case sFOR:
    parse_fr();
    break;
  case sLOOP:
    parse_loop();
    break;
  case sRETURN:
    parse_ret();
    break;
  case sSTART:
    parse_bco();
    break;
  case sIDENTIF:
    if (next_token.category == sABRE_PARENT) {
      parse_call();
    } else {
      parse_atr();
    }
    break;
  default:
    fail("comando");
    break;
  }
}

static void parse_elem(void) { parse_expr(); }

static void parse_litl(void) {
  if (token.category == sSTRING || token.category == sCTEINT ||
      token.category == sCTECHAR || is_bool_literal_token()) {
    advance();
  } else {
    fail("literal");
  }
}

static void parse_primary(void) {
  if (token.category == sSTRING || token.category == sCTEINT ||
      token.category == sCTECHAR || is_bool_literal_token()) {
    parse_litl();
    return;
  }

  if (token.category == sIDENTIF) {
    if (next_token.category == sABRE_PARENT) {
      parse_call();
    } else if (next_token.category == sABRE_COLCH) {
      parse_vec();
    } else {
      parse_id();
    }
    return;
  }

  if (accept(sABRE_PARENT)) {
    parse_expr();
    expect(sFECHA_PARENT, ")");
    return;
  }

  fail("fator");
}

static void parse_fact(void) {
  if (accept(sNEG) || accept(sSUBRAT)) {
    parse_fact();
  } else {
    parse_primary();
  }
}

static void parse_exarp(void) {
  parse_fact();
  while (token.category == sMULT || token.category == sDIV) {
    advance();
    parse_fact();
  }
}

static void parse_exari(void) {
  parse_exarp();
  while (token.category == sSOMA || token.category == sSUBRAT) {
    advance();
    parse_exarp();
  }
}

static void parse_exrel(void) {
  parse_exari();
  while (is_relop(token.category)) {
    advance();
    parse_exari();
  }
}

static void parse_exlog(void) {
  parse_exrel();
  while (accept(sAND)) {
    parse_exrel();
  }
}

static void parse_expr(void) {
  log_trace("parse_expr");
  parse_exlog();
  while (accept(sOR)) {
    parse_exlog();
  }
}

int parser_parse(FILE *source) {
  if (source == NULL) {
    return -1;
  }

  log_trace("inicio_analise_sintatica");

  src = source;
  line_cnt = 1;
  token = lex_next(src, &line_cnt);
  next_token = lex_next(src, &line_cnt);

  parse_ini();
  expect(sEOF, "fim de arquivo");

  log_trace("fim_analise_sintatica");

  return 0;
}
