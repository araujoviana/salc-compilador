/// Flags das opções da linha de comando
typedef struct {
  const char *input_file;
  bool tokens;
  bool symtab;
  bool trace;
} CliOptions;

ArgErr opts_parse(int argc, char *argv[]);
const CliOptions *opts_get(void);
