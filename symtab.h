/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#include "lex.h"

#include <stddef.h>
#include <stdio.h>

#define TS_MAX_SCOPES 256
#define TS_MAX_SYMBOLS 2048

typedef enum {
  SYM_VAR,
  SYM_ARRAY,
  SYM_PROC,
  SYM_FUNC,
  SYM_PARAM,
} SymbolCategory;

typedef enum {
  TYPE_INT,
  TYPE_BOOL,
  TYPE_CHAR,
  TYPE_NONE,
} DataType;

typedef struct {
  char name[LEX_LENGTH];
  SymbolCategory category;
  DataType type;
  int extra;
  int scope_id;
} Symbol;

typedef struct {
  char desc[256];
  int parent_id;
  int block_count;
} Scope;

void ts_init(void);
void ts_destroy(void);
void ts_enter_scope(const char *desc);
void ts_leave_scope(void);
const char *ts_current_scope(void);
void ts_build_block_scope(char *buffer, size_t buffer_size);
int ts_insert(const char *name, SymbolCategory category, DataType type,
              int extra);
Symbol *ts_lookup(const char *name);
void ts_dump(FILE *fp);

#endif
