/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#include "log.h"
#include "opt.h"
#include "parser.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int build_mepa_path(char *buf, size_t buf_size, const char *input_path);

int main(int argc, char *argv[]) {
  // Primeiro confere os argumentos da linha de comando
  ArgErr arg_err = opts_parse(argc, argv);
  if (arg_err != E_OK) {
    log_arg_error(arg_err);
    return EXIT_FAILURE;
  }

  const char *input_file = opts_input_file();
  if (input_file == NULL) {
    log_arg_error(E_OUT_NULL);
    return EXIT_FAILURE;
  }

  FILE *source = fopen(input_file, "r");
  if (source == NULL) {
    log_file_open_error(input_file);
    return EXIT_FAILURE;
  }

  // Se pediu lista de tokens, faz uma passada so do lexico
  if (opts_get(OPT_TOKENS)) {
    if (log_tokens(source) != 0) {
      fclose(source);
      log_shutdown();
      return EXIT_FAILURE;
    }

    if (fseek(source, 0, SEEK_SET) != 0) {
      fprintf(stderr, "Erro: nao foi possivel reposicionar o arquivo.\n");
      fclose(source);
      log_shutdown();
      return EXIT_FAILURE;
    }
  }

  /* Deriva o caminho do arquivo .mepa a partir do arquivo fonte. */
  char mepa_path[1024];
  if (build_mepa_path(mepa_path, sizeof(mepa_path), input_file) != 0) {
    fprintf(stderr, "Erro: caminho do arquivo .mepa excede o limite.\n");
    fclose(source);
    log_shutdown();
    return EXIT_FAILURE;
  }

  FILE *mepa_out = fopen(mepa_path, "w");
  if (mepa_out == NULL) {
    log_file_open_error(mepa_path);
    fclose(source);
    log_shutdown();
    return EXIT_FAILURE;
  }

  // Depois entra na analise sintatica usando a tabela de simbolos
  ts_init();

  if (parse_program(source, mepa_out) != 0) {
    fclose(mepa_out);
    ts_destroy();
    fclose(source);
    log_shutdown();
    return EXIT_FAILURE;
  }

  if (opts_get(OPT_SYMTAB) && log_symtab() != 0) {
    fclose(mepa_out);
    ts_destroy();
    fclose(source);
    log_shutdown();
    return EXIT_FAILURE;
  }

  fclose(mepa_out);
  ts_destroy();
  fclose(source);
  log_shutdown();
  return EXIT_SUCCESS;
}

/* Substitui a extensão do caminho de entrada por ".mepa". */
static int build_mepa_path(char *buf, size_t buf_size, const char *input_path) {
  strncpy(buf, input_path, buf_size - 1);
  buf[buf_size - 1] = '\0';

  char *dot = strrchr(buf, '.');
  if (dot != NULL) {
    *dot = '\0';
  }

  size_t len = strlen(buf);
  if (len + 5 + 1 > buf_size) {
    return -1;
  }
  strncat(buf, ".mepa", buf_size - len - 1);
  return 0;
}