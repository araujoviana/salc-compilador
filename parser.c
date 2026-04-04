/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#include "parser.h"
#include "diag.h"
#include "lex.h"
#include "symtab.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ID_POR_LINHA 64
#define MAX_PARAMS 64

typedef struct {
  char name[LEX_LENGTH];
  int size;
  int line;
} DeclItem;

typedef struct {
  char name[LEX_LENGTH];
  DataType type;
  int extra;
  int line;
} ParamInfo;

typedef enum {
  ID_SCALAR,
  ID_ARRAY,
  ID_SUBROUTINE,
} IdUse;

static Token token;
static Token next_token;
static FILE *src;
static int line_cnt = 1;
static jmp_buf parse_jmp;
static bool in_function = false;
static bool current_function_has_return = false;

static void parse_ini(void);
static void parse_glob(void);
static void parse_subs(void);
static void parse_decls(void);
static void parse_decl_item(DeclItem *item);
static DataType parse_tpo(void);
static DataType parse_tpo_com_tam(int *tam_out);
static void parse_func(void);
static void parse_proc(void);
static void parse_subroutine(bool is_function);
static void parse_princ(void);
static void parse_optional_locals(void);
static int parse_param(ParamInfo buffer[], int max);
static void parse_param_item(ParamInfo *param);
static void parse_decl_sequence(void);
static void insert_params(const ParamInfo params[], int count);
static int parse_expr_list(void);
static void parse_bco(void);
static void parse_cmd(void);
static void parse_out(void);
static void parse_inp(void);
static void parse_if(void);
static void parse_mat(void);
static void parse_wlst(void);
static void parse_whn(void);
static void parse_othr(void);
static void parse_wcnd(void);
static void parse_witem(void);
static void parse_wrnge(void);
static void parse_wint(void);
static void parse_fr(void);
static void parse_wh(void);
static void parse_rpt(void);
static void parse_ret(void);
static void parse_atr(void);
static void parse_call(bool require_function);
static void parse_vec(void);
static void parse_id(void);
static void parse_elem(void);
static void parse_expr(void);
static void parse_exlog(void);
static void parse_exrel(void);
static void parse_exari(void);
static void parse_exarp(void);
static void parse_fact(void);
static void parse_litl(void);

static void copy_text(char *dest, size_t dest_size, const char *src);
static void copy_current_ident(char *dest, size_t dest_size, int *line_out);
static void build_scope_name(char *dest, size_t dest_size, const char *kind,
                             const char *name);
static int parse_positive_int_literal(const char *expected);
static int parse_vector_size_suffix(void);
static void fail_semantic(const char *expected, const char *found, int line);
static bool symbol_matches_use(const Symbol *symbol, IdUse use);
static Symbol *lookup_declared_id(IdUse use, const char *expected);
static Symbol *parse_declared_id(IdUse use, const char *expected);

static void advance(void) {
  // O parser para no primeiro erro para nao espalhar retornos por todo lado
  token = next_token;
  if (token.category == sERROR) {
    longjmp(parse_jmp, 1);
  }

  next_token = lex_next(src, &line_cnt);
  if (next_token.category == sERROR) {
    longjmp(parse_jmp, 1);
  }
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
  longjmp(parse_jmp, 1);
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

// Evita repetir o mesmo teste em varias partes do parser
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

static void copy_text(char *dest, size_t dest_size, const char *src) {
  if (dest == NULL || dest_size == 0) {
    return;
  }

  if (src == NULL) {
    dest[0] = '\0';
    return;
  }

  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
}

static void copy_current_ident(char *dest, size_t dest_size, int *line_out) {
  copy_text(dest, dest_size, token.lexema);
  if (line_out != NULL) {
    *line_out = token.line;
  }
}

static void build_scope_name(char *dest, size_t dest_size, const char *kind,
                             const char *name) {
  size_t prefix_len = 0;
  size_t suffix_len = strlen(".locals");
  size_t name_len = 0;

  if (dest == NULL || dest_size == 0 || kind == NULL || name == NULL) {
    return;
  }

  prefix_len = strlen(kind) + 1;
  if (dest_size <= prefix_len + suffix_len) {
    dest[0] = '\0';
    return;
  }

  name_len = dest_size - prefix_len - suffix_len - 1;
  snprintf(dest, dest_size, "%s:%.*s.locals", kind, (int)name_len, name);
}

static void fail_semantic(const char *expected, const char *found, int line) {
  diag_error(expected, found, line);
  longjmp(parse_jmp, 1);
}

static int parse_positive_int_literal(const char *expected) {
  char lexema[LEX_LENGTH];
  int line = token.line;
  int valor = 0;

  copy_text(lexema, sizeof(lexema), token.lexema);
  expect(sCTEINT, expected);

  valor = atoi(lexema);
  if (valor <= 0) {
    fail_semantic("inteiro positivo", lexema, line);
  }

  return valor;
}

static int parse_vector_size_suffix(void) {
  int size = 0;

  if (!accept(sABRE_COLCH)) {
    return 0;
  }

  size = parse_positive_int_literal("constante inteira");
  expect(sFECHA_COLCH, "]");

  if (token.category == sABRE_COLCH) {
    fail_semantic("apenas um nivel de vetor", token.lexema, token.line);
  }

  return size;
}

static bool symbol_matches_use(const Symbol *sim, IdUse use) {
  if (sim == NULL) {
    return false;
  }

  switch (use) {
  case ID_SCALAR:
    return sim->category == SYM_VAR ||
           (sim->category == SYM_PARAM && sim->extra == 0);
  case ID_ARRAY:
    return sim->category == SYM_ARRAY ||
           (sim->category == SYM_PARAM && sim->extra > 0);
  case ID_SUBROUTINE:
    return sim->category == SYM_PROC || sim->category == SYM_FUNC;
  default:
    return false;
  }
}

static Symbol *lookup_declared_id(IdUse use, const char *expected) {
  char name[LEX_LENGTH];
  int line = 0;
  Symbol *sim;

  copy_current_ident(name, sizeof(name), &line);

  sim = ts_lookup(name);
  if (!symbol_matches_use(sim, use)) {
    fail_semantic(expected, name, line);
  }

  return sim;
}

static Symbol *parse_declared_id(IdUse use, const char *expected) {
  Symbol *sim = lookup_declared_id(use, expected);
  parse_id();
  return sim;
}

static void parse_id(void) { expect(sIDENTIF, "identificador"); }

static void parse_decl_item(DeclItem *item) {
  strncpy(item->name, token.lexema, LEX_LENGTH - 1);
  item->name[LEX_LENGTH - 1] = '\0';
  item->line = token.line;
  expect(sIDENTIF, "identificador");
  item->size = parse_vector_size_suffix();
}

static void parse_decls(void) {
  diag_info("parse_decls");

  DeclItem items[MAX_ID_POR_LINHA];
  int n = 0;
  int type_size = 0;
  DataType type;

  parse_decl_item(&items[n++]);
  while (accept(sVIRGULA)) {
    if (n >= MAX_ID_POR_LINHA) {
      fail_semantic("limite de identificadores por declaracao", token.lexema,
                    token.line);
    }
    parse_decl_item(&items[n++]);
  }

  expect(sDOIS_PTOS, ":");
  type = parse_tpo_com_tam(&type_size);
  expect(sPTO_VIRG, ";");

  for (int i = 0; i < n; i++) {
    if (items[i].size > 0 && type_size > 0) {
      fail_semantic("tamanho de vetor declarado apenas uma vez", items[i].name,
                    items[i].line);
    }

    int tam = (items[i].size > 0) ? items[i].size : type_size;
    SymbolCategory category = (tam > 0) ? SYM_ARRAY : SYM_VAR;
    if (ts_insert(items[i].name, category, type, tam) != 0) {
      fail_semantic("identificador unico no escopo", items[i].name,
                    items[i].line);
    }
  }
}

static void parse_decl_sequence(void) {
  parse_decls();
  while (token.category == sIDENTIF) {
    parse_decls();
  }
}

static DataType parse_tpo_com_tam(int *tam_out) {
  DataType type;
  if (token.category == sINT) {
    type = TYPE_INT;
    advance();
  } else if (is_bool_type_token()) {
    type = TYPE_BOOL;
    advance();
  } else if (token.category == sCHAR) {
    type = TYPE_CHAR;
    advance();
  } else {
    fail("tipo (int|bool|char)");
    type = TYPE_INT;
  }
  int tam = parse_vector_size_suffix();

  if (tam_out)
    *tam_out = tam;

  return type;
}

static DataType parse_tpo(void) { return parse_tpo_com_tam(NULL); }

static void parse_glob(void) {
  diag_info("parse_glob");
  expect(sGLOBALS, "globals");
  parse_decl_sequence();
}

static void parse_optional_locals(void) {
  if (accept(sLOCALS)) {
    parse_decl_sequence();
  }
}

static void parse_param_item(ParamInfo *param) {
  int tam = 0;

  copy_current_ident(param->name, sizeof(param->name), &param->line);
  expect(sIDENTIF, "identificador");
  expect(sDOIS_PTOS, ":");
  param->type = parse_tpo_com_tam(&tam);
  param->extra = tam;
}

static int parse_param(ParamInfo buffer[], int max) {
  int n = 0;

  if (n < max) {
    parse_param_item(&buffer[n]);
  } else {
    fail_semantic("limite de parametros", token.lexema, token.line);
  }
  n++;

  while (accept(sVIRGULA)) {
    if (n < max) {
      parse_param_item(&buffer[n]);
    } else {
      fail_semantic("limite de parametros", token.lexema, token.line);
    }
    n++;
  }

  return n;
}

static void insert_params(const ParamInfo params[], int count) {
  for (int i = 0; i < count; i++) {
    if (ts_insert(params[i].name, SYM_PARAM, params[i].type,
                   params[i].extra) != 0) {
      fail_semantic("parametro unico no escopo", params[i].name,
                    params[i].line);
    }
  }
}

static void parse_subroutine(bool is_function) {
  char name[LEX_LENGTH];
  char scope_name[256];
  ParamInfo params[MAX_PARAMS];
  int name_line = token.line;
  int n_params = 0;
  DataType type = TYPE_NONE;
  bool prev_in_function = in_function;
  bool prev_has_return = current_function_has_return;

  if (is_function) {
    diag_info("parse_func");
    expect(sFN, "fn");
  } else {
    diag_info("parse_proc");
    expect(sPROC, "proc");
  }

  copy_text(name, sizeof(name), token.lexema);
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) {
    n_params = parse_param(params, MAX_PARAMS);
  }
  expect(sFECHA_PARENT, ")");

  if (is_function) {
    expect(sDOIS_PTOS, ":");
    type = parse_tpo();
  }

  if (ts_insert(name, is_function ? SYM_FUNC : SYM_PROC, type, n_params) !=
      0) {
    fail_semantic(is_function ? "funcao unica" : "procedimento unico", name,
                  name_line);
  }

  build_scope_name(scope_name, sizeof(scope_name), is_function ? "fn" : "proc",
                   name);
  ts_enter_scope(scope_name);
  in_function = is_function;
  current_function_has_return = false;
  insert_params(params, n_params);
  parse_optional_locals();
  parse_bco();

  if (is_function && !current_function_has_return) {
    fail_semantic("funcao com comando ret", name, name_line);
  }

  in_function = prev_in_function;
  current_function_has_return = prev_has_return;
  ts_leave_scope();
}

static void parse_func(void) { parse_subroutine(true); }

static void parse_proc(void) { parse_subroutine(false); }

static void parse_subs(void) {
  diag_info("parse_subs");
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
  diag_info("parse_princ");
  int main_line = 0;
  bool prev_in_function = in_function;
  bool prev_has_return = current_function_has_return;

  expect(sPROC, "proc");
  main_line = token.line;
  expect(sMAIN, "main");
  expect(sABRE_PARENT, "(");
  expect(sFECHA_PARENT, ")");

  if (ts_insert("main", SYM_PROC, TYPE_NONE, 0) != 0) {
    fail_semantic("procedimento unico", "main", main_line);
  }

  ts_enter_scope("proc:main.locals");
  in_function = false;
  current_function_has_return = false;
  parse_optional_locals();
  parse_bco();
  in_function = prev_in_function;
  current_function_has_return = prev_has_return;
  ts_leave_scope();
}

static void parse_ini(void) {
  diag_info("parse_ini");
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
  diag_info("parse_bco");

  char desc[256];
  // Cada bloco start end vira um escopo proprio
  ts_build_block_scope(desc, sizeof(desc));
  ts_enter_scope(desc);

  expect(sSTART, "start");
  while (starts_cmd(token.category)) {
    parse_cmd();
    expect(sPTO_VIRG, ";");
  }
  expect(sEND, "end");

  ts_leave_scope();
}

static int parse_expr_list(void) {
  int count = 0;

  if (token.category == sFECHA_PARENT) {
    return 0;
  }

  parse_expr();
  count = 1;
  while (accept(sVIRGULA)) {
    parse_expr();
    count++;
  }

  return count;
}

static void parse_vec(void) {
  parse_declared_id(ID_ARRAY, "vetor declarado");
  expect(sABRE_COLCH, "[");
  if (token.category == sIDENTIF) {
    parse_declared_id(ID_SCALAR, "identificador escalar declarado");
  } else {
    expect(sCTEINT, "constante inteira ou identificador");
  }
  expect(sFECHA_COLCH, "]");
}

static void parse_call(bool require_function) {
  char name[LEX_LENGTH];
  int line = 0;
  int n_args;
  Symbol *sim;

  copy_current_ident(name, sizeof(name), &line);
  sim = parse_declared_id(ID_SUBROUTINE, "sub-rotina declarada");

  if (require_function && sim->category != SYM_FUNC) {
    fail_semantic("funcao declarada", name, line);
  }

  expect(sABRE_PARENT, "(");
  n_args = parse_expr_list();
  expect(sFECHA_PARENT, ")");

  if (sim->extra != n_args) {
    fail_semantic("quantidade correta de parametros", name, line);
  }
}

static void parse_atr(void) {
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_vec();
  } else {
    parse_declared_id(ID_SCALAR,
                      "variavel ou parametro escalar declarado");
  }
  expect(sATRIB, ":=");
  parse_expr();
}

static void parse_out(void) {
  expect(sPRINT, "print");
  expect(sABRE_PARENT, "(");
  parse_expr();
  while (accept(sVIRGULA)) {
    parse_expr();
  }
  expect(sFECHA_PARENT, ")");
}

static void parse_inp(void) {
  expect(sSCAN, "scan");
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_vec();
  } else if (token.category == sIDENTIF) {
    parse_declared_id(ID_SCALAR,
                      "variavel ou parametro escalar declarado");
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

static void parse_wrnge(void) {
  expect(sPTOPTO, "..");
  parse_wint();
}

static void parse_witem(void) {
  parse_wint();
  if (token.category == sPTOPTO) {
    parse_wrnge();
  }
}

static void parse_wcnd(void) {
  parse_witem();
  while (accept(sVIRGULA)) {
    parse_witem();
  }
}

static void parse_whn(void) {
  expect(sWHEN, "when");
  parse_wcnd();
  expect(sIMPLIC, "=>");
  parse_cmd();
  expect(sPTO_VIRG, ";");
}

static void parse_othr(void) {
  expect(sOTHERWISE, "otherwise");
  expect(sIMPLIC, "=>");
  parse_cmd();
  expect(sPTO_VIRG, ";");
}

static void parse_wlst(void) {
  parse_whn();
  while (token.category == sWHEN) {
    parse_whn();
  }
  if (token.category == sOTHERWISE) {
    parse_othr();
  }
}

static void parse_mat(void) {
  expect(sMATCH, "match");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_wlst();
  expect(sEND, "end");
}

static void parse_fr(void) {
  expect(sFOR, "for");
  parse_atr();
  expect(sTO, "to");
  parse_expr();
  if (accept(sSTEP)) {
    if (token.category == sIDENTIF) {
      parse_declared_id(ID_SCALAR, "identificador escalar declarado");
    } else {
      parse_wint();
    }
  }
  expect(sDO, "do");
  parse_cmd();
}

static void parse_wh(void) {
  expect(sLOOP, "loop");
  expect(sWHILE, "while");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_cmd();
}

static void parse_rpt(void) {
  expect(sLOOP, "loop");
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
  if (!in_function) {
    fail_semantic("ret apenas dentro de funcao", token.lexema, token.line);
  }

  expect(sRETURN, "ret");
  current_function_has_return = true;
  parse_expr();
}

static void parse_cmd(void) {
  diag_info("parse_cmd");
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
    if (next_token.category == sWHILE)
      parse_wh();
    else
      parse_rpt();
    break;
  case sRETURN:
    parse_ret();
    break;
  case sSTART:
    parse_bco();
    break;
  case sIDENTIF:
    if (next_token.category == sABRE_PARENT) {
      parse_call(false);
    } else {
      parse_atr();
    }
    break;
  default:
    fail("comando");
    break;
  }
}

static void parse_litl(void) {
  if (token.category == sSTRING || token.category == sCTEINT ||
      token.category == sCTECHAR || is_bool_literal_token()) {
    advance();
  } else {
    fail("literal");
  }
}

static void parse_elem(void) {
  if (token.category == sSTRING || token.category == sCTEINT ||
      token.category == sCTECHAR || is_bool_literal_token()) {
    parse_litl();
    return;
  }

  if (token.category == sIDENTIF) {
    if (next_token.category == sABRE_PARENT) {
      parse_call(true);
    } else if (next_token.category == sABRE_COLCH) {
      parse_vec();
    } else {
      parse_declared_id(ID_SCALAR, "identificador escalar declarado");
    }
    return;
  }

  fail("elemento");
}

static void parse_fact(void) {
  if (accept(sNEG) || accept(sSUBRAT)) {
    parse_fact();
  } else if (accept(sABRE_PARENT)) {
    parse_expr();
    expect(sFECHA_PARENT, ")");
  } else {
    parse_elem();
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
  diag_info("parse_expr");
  parse_exlog();
  while (accept(sOR)) {
    parse_exlog();
  }
}

int parse_program(FILE *source) {
  if (source == NULL) {
    return -1;
  }

  if (setjmp(parse_jmp) != 0) {
    return -1;
  }

  diag_info("inicio_analise_sintatica");

  src = source;
  line_cnt = 1;
  in_function = false;
  current_function_has_return = false;
  token = lex_next(src, &line_cnt);
  next_token = lex_next(src, &line_cnt);

  if (token.category == sERROR || next_token.category == sERROR) {
    return -1;
  }

  parse_ini();
  expect(sEOF, "fim de arquivo");

  diag_info("fim_analise_sintatica");

  return 0;
}
