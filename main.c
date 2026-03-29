#include "log.h"
#include "opt.h"
#include "parser.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
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

  ts_iniciar();

  if (parser_parse(source) != 0) {
    ts_destruir();
    fclose(source);
    log_shutdown();
    return EXIT_FAILURE;
  }

  if (opts_get(OPT_SYMTAB) && log_symtab() != 0) {
    ts_destruir();
    fclose(source);
    log_shutdown();
    return EXIT_FAILURE;
  }

  ts_destruir();
  fclose(source);
  log_shutdown();
  return EXIT_SUCCESS;
}
