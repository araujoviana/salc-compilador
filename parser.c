#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lex.h"
#include "diag.h"

static Token token;
static FILE *src;
static int line_cnt = 1;

static void advance(void) { token = lex_next(src, &line_cnt); }
static void parse_cmd(void);

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

static bool starts_cmd(Category c) {
  return c == sPRINT  ||  c == sSCAN  ||  c == sIF      ||
         c == sMATCH  ||  c == sFOR   ||  c == sLOOP    || 
         c == sRETURN ||  c == sSTART ||  c == sIDENTIF;       
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

static void parse_ini(void) {
  expect(sMODULE, "module");
  parse_id();
  expect(sPTO_VIRG, ";");
  if (token.category == sGLOBALS)
    parse_glob();
  if (token.category == sSUBS) // REVIEW n lembro oq é subs
    parse_subs();
  parse_princ();
}

static void parse_glob(void) {
  expect(sGLOBALS, "globals");
  parse_decls(); // Pelo menos 1 decls
  while (token.category == sIDENTIF) {
    parse_decls();
  }
}

static void parse_subs(void) {
  // TODO ta estranho a explicação
}

static void parse_id(void) { expect(sIDENTIF, token.lexema); }

static void parse_decls(void) {
  parse_id();
  while (token.category == sVIRGULA) {
    expect(sVIRGULA, ",");
    parse_id();
  }
  expect(sDOIS_PTOS, ":");
  parse_tpo();
  expect(sPTO_VIRG, ";");
}

static void parse_tpo(void) {
  switch (token.category) {
  case sINT:
    expect(sINT, "int");
    break;
  case sBOOL:
    expect(sBOOL, "bool");
    break;
  case sCHAR:
    expect(sCHAR, "char");
    break;
  default:
    diag_error("tipo (int|bool|char)", token.lexema, token.line);
    exit(EXIT_FAILURE);
  }

  // REVIEW cade o tpo?
  while (accept(sABRE_COLCH)) {
    expect(sCTEINT, token.lexema);
    expect(sFECHA_COLCH, "]");
  }
}

static void parse_vec(void) {
  parse_id();
  expect(sABRE_COLCH, "[");
  switch (token.category) {
  case sCTEINT:
    expect(sCTEINT, token.lexema); // REVIEW
    break;
  case sIDENTIF:
    parse_id();
    break;
  default:
    diag_error("Constante inteira ou identificador", token.lexema, token.line);
    exit(EXIT_FAILURE);
  }
  expect(sFECHA_COLCH, "]");
}

static void parse_func(void) {
  expect(sFN, "fn");
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) { // REVIEW se é isso, param opcional
    parse_param();
  }
  expect(sFECHA_PARENT, ")");
  expect(sDOIS_PTOS, ":");
  parse_tpo();
  parse_bco();
}

static void parse_proc(void) {
  expect(sPROC, "proc");
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) { // REVIEW se é isso, param opcional
    parse_param();
  }
  expect(sFECHA_PARENT, ")");
  parse_bco();
}

static void parse_princ(void) {
  expect(sPROC, "proc");
  expect(sMAIN, "main");
  parse_id();
  expect(sABRE_PARENT, "(");
  expect(sFECHA_PARENT, ")");
  parse_bco();
}

static void parse_param(void) {
  parse_id();
  expect(sDOIS_PTOS, ":");
  parse_tpo();
  while (accept(sVIRGULA)) {
    expect(sVIRGULA, ",");
    parse_id();
    expect(sDOIS_PTOS, ":");
    parse_tpo();
  }
}

static void parse_cmd(void) {
  switch (token.category)
  {
    case sPRINT:   /* TODO: parse_out() */   break;
    case sSCAN:    /* TODO: parse_inp() */   break;
    case sIF:      parse_if();               break;
    case sMATCH:   /* TODO: parse_mat() */   break;
    case sFOR:     /* TODO: parse_fr() */    break;
    case sLOOP:    /* TODO: parse_wh_ou_rpt() */ break;
    case sRETURN:  /* TODO: parse_ret() */   break;
    case sSTART:   parse_bco();              break;
    case sIDENTIF: {
      Token id = token;  // salva o identificador
      advance();
      if (token.category == sATRIB || token.category == sABRE_COLCH)
        /* TODO: parse_atr(id) */;
      else if (token.category == sABRE_PARENT)
        /* TODO: parse_call(id) */;
      break;
    }
  default:
    diag_error("comando", token.lexema, token.line);
    exit(EXIT_FAILURE);
    break;
  }
}