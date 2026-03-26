#include "log.h"

#include <errno.h>
#include <string.h>

static const char *category_names[] = {
    [sIDENTIF] = "IDENTIF",
    [sCTEINT] = "CTEINT",
    [sCTECHAR] = "CTECHAR",
    [sSTRING] = "STRING",
    [sMODULE] = "MODULE",
    [sGLOBALS] = "GLOBALS",
    [sLOCALS] = "LOCALS",
    [sSTART] = "START",
    [sEND] = "END",
    [sINT] = "INT",
    [sBOOL] = "BOOL",
    [sCHAR] = "CHAR",
    [sFN] = "FN",
    [sPROC] = "PROC",
    [sMAIN] = "MAIN",
    [sRETURN] = "RETURN",
    [sPRINT] = "PRINT",
    [sSCAN] = "SCAN",
    [sIF] = "IF",
    [sELSE] = "ELSE",
    [sMATCH] = "MATCH",
    [sWHEN] = "WHEN",
    [sOTHERWISE] = "OTHERWISE",
    [sFOR] = "FOR",
    [sSTEP] = "STEP",
    [sTO] = "TO",
    [sLOOP] = "LOOP",
    [sWHILE] = "WHILE",
    [sUNTIL] = "UNTIL",
    [sDO] = "DO",
    [sATRIB] = "ATRIB",
    [sIMPLIC] = "IMPLIC",
    [sPTOPTO] = "PTOPTO",
    [sSOMA] = "SOMA",
    [sSUBRAT] = "SUBRAT",
    [sMULT] = "MULT",
    [sDIV] = "DIV",
    [sIGUAL] = "IGUAL",
    [sDIFERENTE] = "DIFERENTE",
    [sMAIOR] = "MAIOR",
    [sMAIORIG] = "MAIORIG",
    [sMENOR] = "MENOR",
    [sMENORIG] = "MENORIG",
    [sOR] = "OR",
    [sAND] = "AND",
    [sNEG] = "NEG",
    [sPTO_VIRG] = "PTO_VIRG",
    [sDOIS_PTOS] = "DOIS_PTOS",
    [sVIRGULA] = "VIRGULA",
    [sABRE_PARENT] = "ABRE_PARENT",
    [sFECHA_PARENT] = "FECHA_PARENT",
    [sABRE_COLCH] = "ABRE_COLCH",
    [sFECHA_COLCH] = "FECHA_COLCH",
    [sEOF] = "EOF",
};

const char *log_category_name(Category category) {
  if (category < 0 || category > sEOF) {
    return "UNKNOWN";
  }

  if (category_names[category] == NULL) {
    return "UNKNOWN";
  }

  return category_names[category];
}

int log_token(const Token *token) {
  if (token == NULL) {
    return -1;
  }

  printf("%4d  %-12s  %s\n", token->line, log_category_name(token->category),
         token->lexema);
  return 0;
}

int log_tokens(FILE *source) {
  if (source == NULL) {
    return -1;
  }

  int line_cnt = 1;
  Token token = {0};

  do {
    token = lex_next(source, &line_cnt);
    log_token(&token);
  } while (token.category != sEOF);

  return 0;
}

int log_trace(const char *message) {
  if (message == NULL) {
    return -1;
  }

  if (!opts_get(OPT_TRACE)) {
    return 0;
  }

  printf("[trace] %s\n", message);
  return 0;
}

int log_arg_error(ArgErr err) {
  switch (err) {
  case E_COUNT:
    fprintf(
        stderr,
        "Uso: ./compilador <arquivo.sal> [--tokens] [--trace] [--symtab]\n");
    fprintf(stderr, "Erro: faltam argumentos obrigatorios.\n");
    return 0;
  case E_PATH:
    fprintf(stderr, "Erro: o arquivo de entrada deve ter extensao .sal.\n");
    return 0;
  case E_FLAG:
    fprintf(
        stderr,
        "Erro: flag invalida. Flags aceitas: --tokens, --trace, --symtab.\n");
    return 0;
  case E_OUT_NULL:
    fprintf(stderr, "Erro interno: ponteiro de argumentos nulo.\n");
    return 0;
  case E_OK:
  default:
    return 0;
  }
}

int log_file_open_error(const char *path) {
  if (path == NULL) {
    fprintf(stderr, "Erro: caminho de arquivo invalido.\n");
    return -1;
  }

  fprintf(stderr, "Erro ao abrir '%s': %s\n", path, strerror(errno));
  return -1;
}
