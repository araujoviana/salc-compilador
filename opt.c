#include "opt.h"

#include <string.h>

static CliOptions
    g_opts; // Argumentos e flags da linha de comando (symtab, trace e tokens)
static bool g_opts_ready =
    false; // Sinaliza se as opções da linha de comando estão prontas e válidas

/// Retorna 0 se o parsing foi correto
ArgErr opts_parse(int argc, char *argv[]) {
  CliOptions tmp = {0};
  g_opts_ready = false;

  // Cláusulas de guarda

  // Argumentos não nulos
  if (argv == NULL)
    return E_OUT_NULL;

  // Número de argumentos correto
  if (argc < 2)
    return E_COUNT;

  // Verifica extensão sal
  const char *extension = strrchr(argv[1], '.');
  if (extension == NULL || strcmp(extension, ".sal") != 0) {
    return E_PATH;
  }

  // Preenche flags no struct
  tmp.input_file = argv[1];

  // Verifica quais flags foram invocadas, pulando arquivo sal
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

// Verifica se a flag requisitada foi chamada pelo usuário
bool opts_get(OptFlag flag) {
  // Flags não disponíveis
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
