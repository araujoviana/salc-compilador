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

static void parse_module(void);
static void parse_globals(void);
static void parse_subroutines(void);
static void parse_declarations(void);
static void parse_decl_item(DeclItem *item);
static DataType parse_type(void);
static DataType parse_type_with_size(int *size_out);
static void parse_function(void);
static void parse_procedure(void);
static void parse_subroutine(bool is_function);
static void parse_main_procedure(void);
static void parse_optional_locals(void);
static int parse_param(ParamInfo buffer[], int max);
static void parse_param_item(ParamInfo *param);
static void parse_declaration_sequence(void);
static void insert_params(const ParamInfo params[], int count);
static int parse_expr_list(void);
static void parse_block(void);
static void parse_command(void);
static void parse_output(void);
static void parse_input(void);
static void parse_if(void);
static void parse_match(void);
static void parse_when_list(void);
static void parse_when_clause(void);
static void parse_otherwise_clause(void);
static void parse_when_condition(void);
static void parse_when_item(void);
static void parse_when_range(void);
static void parse_when_int(void);
static void parse_for(void);
static void parse_while_loop(void);
static void parse_repeat_until(void);
static void parse_return(void);
static void parse_assignment(void);
static void parse_call(bool require_function);
static void parse_array_access(void);
static void parse_id(void);
static void parse_element(void);
static void parse_expr(void);
static void parse_logic_term(void);
static void parse_relation_term(void);
static void parse_add_term(void);
static void parse_mul_term(void);
static void parse_factor(void);
static void parse_literal(void);

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

static bool starts_command(Category c) {
  return c == sPRINT || c == sSCAN || c == sIF || c == sMATCH || c == sFOR ||
         c == sLOOP || c == sRETURN || c == sSTART || c == sIDENTIF;
}

// Evita repetir o mesmo teste em varias partes do parser
static bool is_rel_operator(Category c) {
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
  char lexeme[LEX_LENGTH];
  int line = token.line;
  int value = 0;

  copy_text(lexeme, sizeof(lexeme), token.lexema);
  expect(sCTEINT, expected);

  value = atoi(lexeme);
  if (value <= 0) {
    fail_semantic("inteiro positivo", lexeme, line);
  }

  return value;
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

static void parse_declarations(void) {
  diag_info("parse_declarations");

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
  type = parse_type_with_size(&type_size);
  expect(sPTO_VIRG, ";");

  for (int i = 0; i < n; i++) {
    if (items[i].size > 0 && type_size > 0) {
      fail_semantic("tamanho de vetor declarado apenas uma vez", items[i].name,
                    items[i].line);
    }

    int size = (items[i].size > 0) ? items[i].size : type_size;
    SymbolCategory category = (size > 0) ? SYM_ARRAY : SYM_VAR;
    if (ts_insert(items[i].name, category, type, size) != 0) {
      fail_semantic("identificador unico no escopo", items[i].name,
                    items[i].line);
    }
  }
}

static void parse_declaration_sequence(void) {
  parse_declarations();
  while (token.category == sIDENTIF) {
    parse_declarations();
  }
}

static DataType parse_type_with_size(int *size_out) {
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
  int size = parse_vector_size_suffix();

  if (size_out)
    *size_out = size;

  return type;
}

static DataType parse_type(void) { return parse_type_with_size(NULL); }

static void parse_globals(void) {
  diag_info("parse_globals");
  expect(sGLOBALS, "globals");
  parse_declaration_sequence();
}

static void parse_optional_locals(void) {
  if (accept(sLOCALS)) {
    parse_declaration_sequence();
  }
}

static void parse_param_item(ParamInfo *param) {
  int size = 0;

  copy_current_ident(param->name, sizeof(param->name), &param->line);
  expect(sIDENTIF, "identificador");
  expect(sDOIS_PTOS, ":");
  param->type = parse_type_with_size(&size);
  param->extra = size;
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
  int param_count = 0;
  DataType type = TYPE_NONE;
  bool prev_in_function = in_function;
  bool prev_has_return = current_function_has_return;

  if (is_function) {
    diag_info("parse_function");
    expect(sFN, "fn");
  } else {
    diag_info("parse_procedure");
    expect(sPROC, "proc");
  }

  copy_text(name, sizeof(name), token.lexema);
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) {
    param_count = parse_param(params, MAX_PARAMS);
  }
  expect(sFECHA_PARENT, ")");

  if (is_function) {
    expect(sDOIS_PTOS, ":");
    type = parse_type();
  }

  if (ts_insert(name, is_function ? SYM_FUNC : SYM_PROC, type, param_count) !=
      0) {
    fail_semantic(is_function ? "funcao unica" : "procedimento unico", name,
                  name_line);
  }

  build_scope_name(scope_name, sizeof(scope_name), is_function ? "fn" : "proc",
                   name);
  ts_enter_scope(scope_name);
  in_function = is_function;
  current_function_has_return = false;
  insert_params(params, param_count);
  parse_optional_locals();
  parse_block();

  if (is_function && !current_function_has_return) {
    fail_semantic("funcao com comando ret", name, name_line);
  }

  in_function = prev_in_function;
  current_function_has_return = prev_has_return;
  ts_leave_scope();
}

static void parse_function(void) { parse_subroutine(true); }

static void parse_procedure(void) { parse_subroutine(false); }

static void parse_subroutines(void) {
  diag_info("parse_subroutines");
  while (token.category == sFN ||
         (token.category == sPROC && next_token.category != sMAIN)) {
    if (token.category == sFN) {
      parse_function();
    } else {
      parse_procedure();
    }
  }
}

static void parse_main_procedure(void) {
  diag_info("parse_main_procedure");
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
  parse_block();
  in_function = prev_in_function;
  current_function_has_return = prev_has_return;
  ts_leave_scope();
}

static void parse_module(void) {
  diag_info("parse_module");
  expect(sMODULE, "module");
  parse_id();
  expect(sPTO_VIRG, ";");
  if (token.category == sGLOBALS) {
    parse_globals();
  }
  parse_subroutines();
  parse_main_procedure();
}

static void parse_block(void) {
  diag_info("parse_block");

  char desc[256];
  // Cada bloco start end vira um escopo proprio
  ts_build_block_scope(desc, sizeof(desc));
  ts_enter_scope(desc);

  expect(sSTART, "start");
  while (starts_command(token.category)) {
    parse_command();
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

static void parse_array_access(void) {
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

static void parse_assignment(void) {
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_array_access();
  } else {
    parse_declared_id(ID_SCALAR,
                      "variavel ou parametro escalar declarado");
  }
  expect(sATRIB, ":=");
  parse_expr();
}

static void parse_output(void) {
  expect(sPRINT, "print");
  expect(sABRE_PARENT, "(");
  parse_expr();
  while (accept(sVIRGULA)) {
    parse_expr();
  }
  expect(sFECHA_PARENT, ")");
}

static void parse_input(void) {
  expect(sSCAN, "scan");
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_array_access();
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
  parse_command();
  if (accept(sELSE)) {
    parse_command();
  }
}

static void parse_when_int(void) {
  accept(sSUBRAT);
  expect(sCTEINT, "constante inteira");
}

static void parse_when_range(void) {
  expect(sPTOPTO, "..");
  parse_when_int();
}

static void parse_when_item(void) {
  parse_when_int();
  if (token.category == sPTOPTO) {
    parse_when_range();
  }
}

static void parse_when_condition(void) {
  parse_when_item();
  while (accept(sVIRGULA)) {
    parse_when_item();
  }
}

static void parse_when_clause(void) {
  expect(sWHEN, "when");
  parse_when_condition();
  expect(sIMPLIC, "=>");
  parse_command();
  expect(sPTO_VIRG, ";");
}

static void parse_otherwise_clause(void) {
  expect(sOTHERWISE, "otherwise");
  expect(sIMPLIC, "=>");
  parse_command();
  expect(sPTO_VIRG, ";");
}

static void parse_when_list(void) {
  parse_when_clause();
  while (token.category == sWHEN) {
    parse_when_clause();
  }
  if (token.category == sOTHERWISE) {
    parse_otherwise_clause();
  }
}

static void parse_match(void) {
  expect(sMATCH, "match");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_when_list();
  expect(sEND, "end");
}

static void parse_for(void) {
  expect(sFOR, "for");
  parse_assignment();
  expect(sTO, "to");
  parse_expr();
  if (accept(sSTEP)) {
    if (token.category == sIDENTIF) {
      parse_declared_id(ID_SCALAR, "identificador escalar declarado");
    } else {
      parse_when_int();
    }
  }
  expect(sDO, "do");
  parse_command();
}

static void parse_while_loop(void) {
  expect(sLOOP, "loop");
  expect(sWHILE, "while");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
  parse_command();
}

static void parse_repeat_until(void) {
  expect(sLOOP, "loop");
  while (starts_command(token.category)) {
    parse_command();
    expect(sPTO_VIRG, ";");
  }
  expect(sUNTIL, "until");
  expect(sABRE_PARENT, "(");
  parse_expr();
  expect(sFECHA_PARENT, ")");
}

static void parse_return(void) {
  if (!in_function) {
    fail_semantic("ret apenas dentro de funcao", token.lexema, token.line);
  }

  expect(sRETURN, "ret");
  current_function_has_return = true;
  parse_expr();
}

static void parse_command(void) {
  diag_info("parse_command");
  switch (token.category) {
  case sPRINT:
    parse_output();
    break;
  case sSCAN:
    parse_input();
    break;
  case sIF:
    parse_if();
    break;
  case sMATCH:
    parse_match();
    break;
  case sFOR:
    parse_for();
    break;
  case sLOOP:
    if (next_token.category == sWHILE)
      parse_while_loop();
    else
      parse_repeat_until();
    break;
  case sRETURN:
    parse_return();
    break;
  case sSTART:
    parse_block();
    break;
  case sIDENTIF:
    if (next_token.category == sABRE_PARENT) {
      parse_call(false);
    } else {
      parse_assignment();
    }
    break;
  default:
    fail("comando");
    break;
  }
}

static void parse_literal(void) {
  if (token.category == sSTRING || token.category == sCTEINT ||
      token.category == sCTECHAR || is_bool_literal_token()) {
    advance();
  } else {
    fail("literal");
  }
}

static void parse_element(void) {
  if (token.category == sSTRING || token.category == sCTEINT ||
      token.category == sCTECHAR || is_bool_literal_token()) {
    parse_literal();
    return;
  }

  if (token.category == sIDENTIF) {
    if (next_token.category == sABRE_PARENT) {
      parse_call(true);
    } else if (next_token.category == sABRE_COLCH) {
      parse_array_access();
    } else {
      parse_declared_id(ID_SCALAR, "identificador escalar declarado");
    }
    return;
  }

  fail("elemento");
}

static void parse_factor(void) {
  if (accept(sNEG) || accept(sSUBRAT)) {
    parse_factor();
  } else if (accept(sABRE_PARENT)) {
    parse_expr();
    expect(sFECHA_PARENT, ")");
  } else {
    parse_element();
  }
}

static void parse_mul_term(void) {
  parse_factor();
  while (token.category == sMULT || token.category == sDIV) {
    advance();
    parse_factor();
  }
}

static void parse_add_term(void) {
  parse_mul_term();
  while (token.category == sSOMA || token.category == sSUBRAT) {
    advance();
    parse_mul_term();
  }
}

static void parse_relation_term(void) {
  parse_add_term();
  while (is_rel_operator(token.category)) {
    advance();
    parse_add_term();
  }
}

static void parse_logic_term(void) {
  parse_relation_term();
  while (accept(sAND)) {
    parse_relation_term();
  }
}

static void parse_expr(void) {
  diag_info("parse_expr");
  parse_logic_term();
  while (accept(sOR)) {
    parse_logic_term();
  }
}

int parse_program(FILE *source, FILE *mepa_out) {
  if (source == NULL) {
    return -1;
  }

  if (setjmp(parse_jmp) != 0) {
    return -1;
  }

  gen_init(mepa_out);

  diag_info("inicio_analise_sintatica");

  src = source;
  line_cnt = 1;
  in_function = false;
  current_function_has_return = false;
  g_current_function_type = TYPE_NONE;
  g_addr_counter = 0;
  token = lex_next(src, &line_cnt);
  next_token = lex_next(src, &line_cnt);

  if (token.category == sERROR || next_token.category == sERROR) {
    return -1;
  }

  parse_module();
  expect(sEOF, "fim de arquivo");

  diag_info("fim_analise_sintatica");

  return 0;
}
