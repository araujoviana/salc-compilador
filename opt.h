/*
 * Projeto SALc - Fase 1
 * Arquivo: opt.h
 * Integrantes:
 * - Matheus Gabriel Viana Araujo - 10420444
 * - Luis Fernando de Mesquita Pereira - 10410686
 */

#ifndef OPT_H
#define OPT_H

#include <stdbool.h>

// Estrutura simples com as opcoes da linha de comando.
typedef struct {
  const char *input_file;
  bool tokens;
  bool symtab;
  bool trace;
} CliOptions;

// Enum usado para consultar uma opcao especifica.
typedef enum { OPT_TOKENS, OPT_SYMTAB, OPT_TRACE } OptFlag;

// Possiveis erros encontrados na leitura dos argumentos.
typedef enum {
  E_OK = 0,
  E_COUNT,
  E_PATH,
  E_FLAG,
  E_OUT_NULL,
} ArgErr;

ArgErr opts_parse(int argc, char *argv[]);
bool opts_get(OptFlag flag);
const char *opts_input_file(void);

#endif
