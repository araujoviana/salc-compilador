/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#include "symtab.h"

#include <stdio.h>
#include <string.h>

// Vetores fixos deixam a tabela mais simples para esse projeto
static Scope scopes[TS_MAX_SCOPES];
static Symbol symbols[TS_MAX_SYMBOLS];
static int scope_count = 0;
static int symbol_count = 0;
static int current_scope_id = -1;

static const char *category_to_string(SymbolCategory category) {
  switch (category) {
  case SYM_VAR:
    return "var";
  case SYM_ARRAY:
    return "vetor";
  case SYM_PROC:
    return "proc";
  case SYM_FUNC:
    return "funcao";
  case SYM_PARAM:
    return "param";
  default:
    return "?";
  }
}

static const char *type_to_string(DataType type) {
  switch (type) {
  case TYPE_INT:
    return "int";
  case TYPE_BOOL:
    return "bool";
  case TYPE_CHAR:
    return "char";
  case TYPE_NONE:
    return "nenhum";
  default:
    return "?";
  }
}

void ts_init(void) {
  scope_count = 0;
  symbol_count = 0;
  current_scope_id = -1;
  ts_enter_scope("global");
}

void ts_destroy(void) {
  scope_count = 0;
  symbol_count = 0;
  current_scope_id = -1;
}

void ts_enter_scope(const char *desc) {
  Scope *scope;

  if (scope_count >= TS_MAX_SCOPES) {
    fprintf(stderr, "Erro: limite de escopos excedido\n");
    return;
  }

  scope = &scopes[scope_count];
  strncpy(scope->desc, desc, sizeof(scope->desc) - 1);
  scope->desc[sizeof(scope->desc) - 1] = '\0';
  scope->parent_id = current_scope_id;
  scope->block_count = 0;

  current_scope_id = scope_count;
  scope_count++;
}

void ts_leave_scope(void) {
  if (current_scope_id < 0) {
    return;
  }

  current_scope_id = scopes[current_scope_id].parent_id;
}

const char *ts_current_scope(void) {
  if (current_scope_id < 0) {
    return "(none)";
  }

  return scopes[current_scope_id].desc;
}

void ts_build_block_scope(char *buffer, size_t buffer_size) {
  int base_scope_id = current_scope_id;
  char base_desc[256];
  char *locals_part;

  if (current_scope_id < 0) {
    snprintf(buffer, buffer_size, "block#?");
    return;
  }

  // O contador do bloco fica preso ao escopo local mais proximo
  while (base_scope_id >= 0 &&
         strstr(scopes[base_scope_id].desc, ".locals") == NULL) {
    base_scope_id = scopes[base_scope_id].parent_id;
  }

  if (base_scope_id < 0) {
    base_scope_id = current_scope_id;
  }

  scopes[base_scope_id].block_count++;

  strncpy(base_desc, scopes[base_scope_id].desc, sizeof(base_desc) - 1);
  base_desc[sizeof(base_desc) - 1] = '\0';

  locals_part = strstr(base_desc, ".locals");
  if (locals_part != NULL) {
    *locals_part = '\0';
  }

  snprintf(buffer, buffer_size, "%s.block#%d", base_desc,
           scopes[base_scope_id].block_count);
}

int ts_insert(const char *name, SymbolCategory category, DataType type,
              int extra) {
  Symbol *symbol;

  if (current_scope_id < 0 || symbol_count >= TS_MAX_SYMBOLS) {
    return -1;
  }

  for (int i = 0; i < symbol_count; i++) {
    if (symbols[i].scope_id == current_scope_id &&
        strcmp(symbols[i].name, name) == 0) {
      return -1;
    }
  }

  symbol = &symbols[symbol_count];
  strncpy(symbol->name, name, sizeof(symbol->name) - 1);
  symbol->name[sizeof(symbol->name) - 1] = '\0';
  symbol->category = category;
  symbol->type = type;
  symbol->extra = extra;
  symbol->scope_id = current_scope_id;

  symbol_count++;
  return 0;
}

Symbol *ts_lookup(const char *name) {
  int scope_id = current_scope_id;

  while (scope_id >= 0) {
    for (int i = 0; i < symbol_count; i++) {
      if (symbols[i].scope_id == scope_id && strcmp(symbols[i].name, name) == 0) {
        return &symbols[i];
      }
    }

    scope_id = scopes[scope_id].parent_id;
  }

  return NULL;
}

void ts_dump(FILE *fp) {
  for (int scope_id = 0; scope_id < scope_count; scope_id++) {
    for (int i = 0; i < symbol_count; i++) {
      if (symbols[i].scope_id != scope_id) {
        continue;
      }

      fprintf(fp, "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s  extra=%d\n",
              scopes[scope_id].desc, symbols[i].name,
              category_to_string(symbols[i].category),
              type_to_string(symbols[i].type), symbols[i].extra);
    }
  }
}
