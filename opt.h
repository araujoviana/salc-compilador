#ifndef OPT_H
#define OPT_H

#include <stdbool.h>

/// Flags das opções da linha de comando
typedef struct {
  const char *input_file;
  bool tokens;
  bool symtab;
  bool trace;
} CliOptions;

// Criado pra faciliar a interface de opts_get
typedef enum { OPT_TOKENS, OPT_SYMTAB, OPT_TRACE } OptFlag;

// Tipos de erro retornado pelo arquivo
typedef enum {
  E_OK = 0,
  E_COUNT,
  E_PATH,
  E_FLAG,
  E_OUT_NULL,
} ArgErr;

ArgErr opts_parse(int argc, char *argv[]);
bool opts_get(OptFlag flag);

#endif
