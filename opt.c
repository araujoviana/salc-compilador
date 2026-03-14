#include <stdbool.h>
#include <string.h>

static CliOptions g_opts; // Argumentos e flags da linha de comando
static bool g_opts_ready = false;

// Tipos de erro retornado pelo arquivo
typedef enum {
  E_OK = 0,
  E_COUNT,
  E_PATH,
  E_FLAG,
  E_OUT_NULL,
} ArgErr;

/// Retorna 0 se o parsing foi correto
ArgErr opts_parse(int argc, char *argv[], CliOptions *out) {
  CliOptions tmp{0};

  // Argumentos não nulos
  if (out == NULL || argv == NULL)
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
  *out = (CliOptions){
      .input_file = argv[1],
      .tokens = false,
      .symtab = false,
      .trace = false,
  };

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--tokens") == 0) {
      out->tokens = true;
    } else if (strcmp(argv[i], "--symtab") == 0) {
      out->symtab = true;
    } else if (strcmp(argv[i], "--trace") == 0) {
      out->trace = true;
    } else {
      return E_FLAG;
    }
  }

  g_opts = tmp;
  g_opts_ready = true;
  return E_OK;
}

// Verifica se a flag requisitada foi chamada pelo usuário
const CliOptions *opts_get(void) { return g_opts_ready ? &g_opts : NULL; }
