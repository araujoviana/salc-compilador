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

// Tipos de erro retornado pelo arquivo
typedef enum {
  E_OK = 0,
  E_COUNT,
  E_PATH,
  E_FLAG,
  E_OUT_NULL,
} ArgErr;

ArgErr opts_parse(int argc, char *argv[]);
const CliOptions *opts_get(void);

#endif
