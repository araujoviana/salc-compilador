#include "parser.h"
#include "diag.h"
#include "lex.h"
#include "symtab.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ID_POR_LINHA 64
#define MAX_PARAMS 64

typedef struct {
  char nome[LEX_LENGTH];
  int tamanho;
  int linha;
} ItemDecl;

typedef struct {
  char nome[LEX_LENGTH];
  Tipo tipo;
  int extra;
  int linha;
} InfoParam;

typedef enum {
  ID_ESCALAR,
  ID_VETOR,
  ID_SUBROTINA,
} IdUso;

static Token token;
static Token next_token;
static FILE *src;
static int line_cnt = 1;

static void parse_ini(void);
static void parse_glob(void);
static void parse_subs(void);
static void parse_decls(void);
static void parse_decl_item(ItemDecl *item);
static Tipo parse_tpo(void);
static Tipo parse_tpo_com_tam(int *tam_out);
static void parse_func(void);
static void parse_proc(void);
static void parse_subrotina(bool is_funcao);
static void parse_princ(void);
static void parse_locals_opt(void);
static int parse_param(InfoParam buf[], int max);
static void parse_param_item(InfoParam *param);
static void parse_decls_seq(void);
static void inserir_params(const InfoParam params[], int count);
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
static void parse_litl(void);

static void copy_text(char *dest, size_t dest_size, const char *src);
static void copy_ident_atual(char *dest, size_t dest_size, int *line_out);
static void build_scope_desc(char *dest, size_t dest_size, const char *kind,
                             const char *name);
static void fail_semantic(const char *expected, const char *found, int line);
static bool simbolo_compativel(const Simbolo *sim, IdUso uso);
static Simbolo *buscar_id_declarado(IdUso uso, const char *expected);
static Simbolo *parse_id_declarado(IdUso uso, const char *expected);

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

// Verifica se o token é um operador relacional
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

static void copy_ident_atual(char *dest, size_t dest_size, int *line_out) {
  copy_text(dest, dest_size, token.lexema);
  if (line_out != NULL) {
    *line_out = token.line;
  }
}

static void build_scope_desc(char *dest, size_t dest_size, const char *kind,
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
  exit(EXIT_FAILURE);
}

static bool simbolo_compativel(const Simbolo *sim, IdUso uso) {
  if (sim == NULL) {
    return false;
  }

  switch (uso) {
  case ID_ESCALAR:
    return sim->cat == CAT_VAR || (sim->cat == CAT_PARAM && sim->extra == 0);
  case ID_VETOR:
    return sim->cat == CAT_VETOR ||
           (sim->cat == CAT_PARAM && sim->extra > 0);
  case ID_SUBROTINA:
    return sim->cat == CAT_PROC || sim->cat == CAT_FUNCAO;
  default:
    return false;
  }
}

static Simbolo *buscar_id_declarado(IdUso uso, const char *expected) {
  char nome[LEX_LENGTH];
  int linha = 0;
  Simbolo *sim;

  copy_ident_atual(nome, sizeof(nome), &linha);

  sim = ts_buscar(nome);
  if (!simbolo_compativel(sim, uso)) {
    fail_semantic(expected, nome, linha);
  }

  return sim;
}

static Simbolo *parse_id_declarado(IdUso uso, const char *expected) {
  Simbolo *sim = buscar_id_declarado(uso, expected);
  parse_id();
  return sim;
}

static void parse_id(void) { expect(sIDENTIF, "identificador"); }

static void parse_decl_item(ItemDecl *item) {
  strncpy(item->nome, token.lexema, LEX_LENGTH - 1);
  item->nome[LEX_LENGTH - 1] = '\0';
  item->linha = token.line;
  expect(sIDENTIF, "identificador");
  item->tamanho = 0;
  if (accept(sABRE_COLCH)) {
    item->tamanho = atoi(token.lexema);
    expect(sCTEINT, "constante inteira");
    expect(sFECHA_COLCH, "]");
  }
}

static void parse_decls(void) {
  diag_info("parse_decls");

  ItemDecl itens[MAX_ID_POR_LINHA];
  int n = 0;
  int tam_tipo = 0;
  Tipo tipo;

  parse_decl_item(&itens[n++]);
  while (accept(sVIRGULA)) {
    if (n >= MAX_ID_POR_LINHA) {
      fail_semantic("limite de identificadores por declaracao", token.lexema,
                    token.line);
    }
    parse_decl_item(&itens[n++]);
  }

  expect(sDOIS_PTOS, ":");
  tipo = parse_tpo_com_tam(&tam_tipo);
  expect(sPTO_VIRG, ";");

  for (int i = 0; i < n; i++) {
    int tam = (itens[i].tamanho > 0) ? itens[i].tamanho : tam_tipo;
    Categoria cat = (tam > 0) ? CAT_VETOR : CAT_VAR;
    if (ts_inserir(itens[i].nome, cat, tipo, tam) != 0) {
      fail_semantic("identificador unico no escopo", itens[i].nome,
                    itens[i].linha);
    }
  }
}

static void parse_decls_seq(void) {
  parse_decls();
  while (token.category == sIDENTIF) {
    parse_decls();
  }
}

static Tipo parse_tpo_com_tam(int *tam_out) {
  Tipo t;
  if (token.category == sINT) {
    t = TIPO_INT;
    advance();
  } else if (is_bool_type_token()) {
    t = TIPO_BOOL;
    advance();
  } else if (token.category == sCHAR) {
    t = TIPO_CHAR;
    advance();
  } else {
    fail("tipo (int|bool|char)");
    t = TIPO_INT;
  }
  int tam = 0;

  while (accept(sABRE_COLCH)) {
    tam = atoi(token.lexema);
    expect(sCTEINT, "constante inteira");
    expect(sFECHA_COLCH, "]");
  }

  if (tam_out)
    *tam_out = tam;

  return t;
}

static Tipo parse_tpo(void) { return parse_tpo_com_tam(NULL); }

static void parse_glob(void) {
  diag_info("parse_glob");
  expect(sGLOBALS, "globals");
  parse_decls_seq();
}

static void parse_locals_opt(void) {
  if (accept(sLOCALS)) {
    parse_decls_seq();
  }
}

static void parse_param_item(InfoParam *param) {
  int tam = 0;

  copy_ident_atual(param->nome, sizeof(param->nome), &param->linha);
  expect(sIDENTIF, "identificador");
  expect(sDOIS_PTOS, ":");
  param->tipo = parse_tpo_com_tam(&tam);
  param->extra = tam;
}

static int parse_param(InfoParam buf[], int max) {
  int n = 0;

  if (n < max) {
    parse_param_item(&buf[n]);
  } else {
    fail_semantic("limite de parametros", token.lexema, token.line);
  }
  n++;

  while (accept(sVIRGULA)) {
    if (n < max) {
      parse_param_item(&buf[n]);
    } else {
      fail_semantic("limite de parametros", token.lexema, token.line);
    }
    n++;
  }

  return n;
}

static void inserir_params(const InfoParam params[], int count) {
  for (int i = 0; i < count; i++) {
    if (ts_inserir(params[i].nome, CAT_PARAM, params[i].tipo,
                   params[i].extra) != 0) {
      fail_semantic("parametro unico no escopo", params[i].nome,
                    params[i].linha);
    }
  }
}

static void parse_subrotina(bool is_funcao) {
  char nome[LEX_LENGTH];
  char desc[256];
  InfoParam params[MAX_PARAMS];
  int nome_linha = token.line;
  int n_params = 0;
  Tipo tipo = TIPO_NENHUM;

  if (is_funcao) {
    diag_info("parse_func");
    expect(sFN, "fn");
  } else {
    diag_info("parse_proc");
    expect(sPROC, "proc");
  }

  copy_text(nome, sizeof(nome), token.lexema);
  parse_id();
  expect(sABRE_PARENT, "(");
  if (token.category == sIDENTIF) {
    n_params = parse_param(params, MAX_PARAMS);
  }
  expect(sFECHA_PARENT, ")");

  if (is_funcao) {
    expect(sDOIS_PTOS, ":");
    tipo = parse_tpo();
  }

  if (ts_inserir(nome, is_funcao ? CAT_FUNCAO : CAT_PROC, tipo, n_params) !=
      0) {
    fail_semantic(is_funcao ? "funcao unica" : "procedimento unico", nome,
                  nome_linha);
  }

  build_scope_desc(desc, sizeof(desc), is_funcao ? "fn" : "proc", nome);
  ts_entrar_escopo(desc);
  inserir_params(params, n_params);
  parse_locals_opt();
  parse_bco();
  ts_sair_escopo();
}

static void parse_func(void) { parse_subrotina(true); }

static void parse_proc(void) { parse_subrotina(false); }

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
  int main_linha = 0;

  expect(sPROC, "proc");
  main_linha = token.line;
  expect(sMAIN, "main");
  expect(sABRE_PARENT, "(");
  expect(sFECHA_PARENT, ")");

  if (ts_inserir("main", CAT_PROC, TIPO_NENHUM, 0) != 0) {
    fail_semantic("procedimento unico", "main", main_linha);
  }

  ts_entrar_escopo("proc:main.locals");
  parse_locals_opt();
  parse_bco();
  ts_sair_escopo();
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
  ts_desc_bloco(desc, sizeof(desc));
  ts_entrar_escopo(desc);

  expect(sSTART, "start");
  while (starts_cmd(token.category)) {
    parse_cmd();
    expect(sPTO_VIRG, ";");
  }
  expect(sEND, "end");

  ts_sair_escopo();
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
  parse_id_declarado(ID_VETOR, "vetor declarado");
  expect(sABRE_COLCH, "[");
  if (token.category == sIDENTIF) {
    parse_id_declarado(ID_ESCALAR, "identificador escalar declarado");
  } else {
    expect(sCTEINT, "constante inteira ou identificador");
  }
  expect(sFECHA_COLCH, "]");
}

static void parse_call(void) {
  char nome[LEX_LENGTH];
  int linha = 0;
  int n_args;
  Simbolo *sim;

  copy_ident_atual(nome, sizeof(nome), &linha);
  sim = parse_id_declarado(ID_SUBROTINA, "sub-rotina declarada");
  expect(sABRE_PARENT, "(");
  n_args = parse_expr_list();
  expect(sFECHA_PARENT, ")");

  if (sim->extra != n_args) {
    fail_semantic("quantidade correta de parametros", nome, linha);
  }
}

static void parse_atr(void) {
  if (token.category == sIDENTIF && next_token.category == sABRE_COLCH) {
    parse_vec();
  } else {
    parse_id_declarado(ID_ESCALAR,
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
    parse_id_declarado(ID_ESCALAR,
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
      parse_id_declarado(ID_ESCALAR, "identificador escalar declarado");
    } else {
      expect(sCTEINT, "constante inteira");
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
  expect(sRETURN, "ret");
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
      parse_call();
    } else if (next_token.category == sABRE_COLCH) {
      parse_vec();
    } else {
      parse_id_declarado(ID_ESCALAR, "identificador escalar declarado");
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

int parser_parse(FILE *source) {
  if (source == NULL) {
    return -1;
  }

  diag_info("inicio_analise_sintatica");

  src = source;
  line_cnt = 1;
  token = lex_next(src, &line_cnt);
  next_token = lex_next(src, &line_cnt);

  parse_ini();
  expect(sEOF, "fim de arquivo");

  diag_info("fim_analise_sintatica");

  return 0;
}
