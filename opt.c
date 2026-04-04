/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#include "opt.h"

#include <string.h>

static CliOptions
    g_opts; // Guarda o arquivo de entrada e as opcoes da execucao
static bool g_opts_ready =
    false; // Diz se a leitura da linha de comando ja foi feita

// Faz a leitura dos argumentos e valida o formato basico
ArgErr opts_parse(int argc, char *argv[]) {
  CliOptions tmp = {0};
  g_opts_ready = false;

  // Clausulas de guarda

  // O vetor de argumentos precisa existir
  if (argv == NULL)
    return E_OUT_NULL;

  // Precisa ter pelo menos o nome do programa e o arquivo SAL
  if (argc < 2)
    return E_COUNT;

  // O arquivo de entrada precisa terminar com extensao SAL
  const char *extension = strrchr(argv[1], '.');
  if (extension == NULL || strcmp(extension, ".sal") != 0) {
    return E_PATH;
  }

  // Guarda o caminho do arquivo fonte
  tmp.input_file = argv[1];

  // Le as opcoes depois do nome do arquivo
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--tokens") == 0) {
      tmp.tokens = true;
    } else if (strcmp(argv[i], "--symtab") == 0) {
      tmp.symtab = true;
    } else if (strcmp(argv[i], "--trace") == 0) {
      tmp.trace = true;
    } else {
      return E_FLAG;
    }
  }

  g_opts = tmp;
  g_opts_ready = true;
  return E_OK;
}

// Consulta uma opcao ja lida no parsing
bool opts_get(OptFlag flag) {
  // Se nao houve parsing ainda, nao ha o que consultar
  if (!g_opts_ready)
    return false;

  switch (flag) {
  case OPT_TOKENS:
    return g_opts.tokens;
  case OPT_SYMTAB:
    return g_opts.symtab;
  case OPT_TRACE:
    return g_opts.trace;
  default:
    return false;
  }
}

const char *opts_input_file(void) {
  if (!g_opts_ready) {
    return NULL;
  }

  return g_opts.input_file;
}
