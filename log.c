#include "log.h"

#include "symtab.h"

#include <errno.h>
#include <string.h>

static const char *category_names[] = {
    [sIDENTIF] = "sIDENTIF",
    [sCTEINT] = "sCTEINT",
    [sCTECHAR] = "sCTECHAR",
    [sSTRING] = "sSTRING",
    [sMODULE] = "sMODULE",
    [sGLOBALS] = "sGLOBALS",
    [sLOCALS] = "sLOCALS",
    [sSTART] = "sSTART",
    [sEND] = "sEND",
    [sINT] = "sINT",
    [sBOOL] = "sBOOL",
    [sCHAR] = "sCHAR",
    [sFN] = "sFN",
    [sPROC] = "sPROC",
    [sMAIN] = "sMAIN",
    [sRETURN] = "sRETURN",
    [sPRINT] = "sPRINT",
    [sSCAN] = "sSCAN",
    [sIF] = "sIF",
    [sELSE] = "sELSE",
    [sMATCH] = "sMATCH",
    [sWHEN] = "sWHEN",
    [sOTHERWISE] = "sOTHERWISE",
    [sFOR] = "sFOR",
    [sSTEP] = "sSTEP",
    [sTO] = "sTO",
    [sLOOP] = "sLOOP",
    [sWHILE] = "sWHILE",
    [sUNTIL] = "sUNTIL",
    [sDO] = "sDO",
    [sATRIB] = "sATRIB",
    [sIMPLIC] = "sIMPLIC",
    [sPTOPTO] = "sPTOPTO",
    [sSOMA] = "sSOMA",
    [sSUBRAT] = "sSUBRAT",
    [sMULT] = "sMULT",
    [sDIV] = "sDIV",
    [sIGUAL] = "sIGUAL",
    [sDIFERENTE] = "sDIFERENTE",
    [sMAIOR] = "sMAIOR",
    [sMAIORIG] = "sMAIORIG",
    [sMENOR] = "sMENOR",
    [sMENORIG] = "sMENORIG",
    [sOR] = "sOR",
    [sAND] = "sAND",
    [sNEG] = "sNEG",
    [sPTO_VIRG] = "sPTO_VIRG",
    [sDOIS_PTOS] = "sDOIS_PTOS",
    [sVIRGULA] = "sVIRGULA",
    [sABRE_PARENT] = "sABRE_PARENT",
    [sFECHA_PARENT] = "sFECHA_PARENT",
    [sABRE_COLCH] = "sABRE_COLCH",
    [sFECHA_COLCH] = "sFECHA_COLCH",
    [sEOF] = "sEOF",
};

static FILE *g_trace_file = NULL;

static int build_output_path(char *buffer, size_t buffer_size,
                             const char *extension) {
  const char *input_file = opts_input_file();
  char *dot = NULL;

  if (buffer == NULL || buffer_size == 0 || extension == NULL ||
      input_file == NULL) {
    return -1;
  }

  strncpy(buffer, input_file, buffer_size - 1);
  buffer[buffer_size - 1] = '\0';

  dot = strrchr(buffer, '.');
  if (dot != NULL) {
    *dot = '\0';
  }

  if (strlen(buffer) + strlen(extension) + 1 > buffer_size) {
    return -1;
  }

  strcat(buffer, extension);
  return 0;
}

static int write_token(FILE *output, const Token *token) {
  if (output == NULL || token == NULL) {
    return -1;
  }

  fprintf(output, "%d  %s  \"%s\"\n", token->line,
          log_category_name(token->category), token->lexema);
  return 0;
}

static FILE *trace_file_open(void) {
  char path[1024];

  if (g_trace_file != NULL) {
    return g_trace_file;
  }

  if (!opts_get(OPT_TRACE)) {
    return NULL;
  }

  if (build_output_path(path, sizeof(path), ".trc") != 0) {
    return NULL;
  }

  g_trace_file = fopen(path, "w");
  if (g_trace_file == NULL) {
    log_file_open_error(path);
  }

  return g_trace_file;
}

const char *log_category_name(Category category) {
  if (category < 0 || category > sEOF) {
    return "UNKNOWN";
  }

  if (category_names[category] == NULL) {
    return "UNKNOWN";
  }

  return category_names[category];
}

int log_token(const Token *token) { return write_token(stdout, token); }

int log_tokens(FILE *source) {
  char path[1024];
  int line_cnt = 1;
  Token token = {0};
  FILE *output = NULL;

  if (source == NULL) {
    return -1;
  }

  if (build_output_path(path, sizeof(path), ".tk") != 0) {
    return -1;
  }

  output = fopen(path, "w");
  if (output == NULL) {
    return log_file_open_error(path);
  }

  do {
    token = lex_next(source, &line_cnt);
    write_token(output, &token);
  } while (token.category != sEOF);

  fclose(output);
  return 0;
}

int log_symtab(void) {
  char path[1024];
  FILE *output = NULL;

  if (build_output_path(path, sizeof(path), ".ts") != 0) {
    return -1;
  }

  output = fopen(path, "w");
  if (output == NULL) {
    return log_file_open_error(path);
  }

  ts_dump(output);
  fclose(output);
  return 0;
}

int log_trace(const char *message) {
  FILE *output = NULL;

  if (message == NULL) {
    return -1;
  }

  if (!opts_get(OPT_TRACE)) {
    return 0;
  }

  output = trace_file_open();
  if (output == NULL) {
    return -1;
  }

  fprintf(output, "%s\n", message);
  return 0;
}

int log_shutdown(void) {
  if (g_trace_file != NULL) {
    fclose(g_trace_file);
    g_trace_file = NULL;
  }

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
