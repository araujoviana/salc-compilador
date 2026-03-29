#ifndef LOG_H
#define LOG_H

#include "lex.h"
#include "opt.h"

#include <stdio.h>

const char *log_category_name(Category category);
int log_token(const Token *token);
int log_tokens(FILE *source);
int log_symtab(void);
int log_trace(const char *message);
int log_shutdown(void);
int log_arg_error(ArgErr err);
int log_file_open_error(const char *path);

#endif
