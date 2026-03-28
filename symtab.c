#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

typedef struct LogEscopo {
    Escopo           *escopo;
    struct LogEscopo *prox;
} LogEscopo;

static Escopo    *atual = NULL; 
static LogEscopo *cabeca_log = NULL;    
static LogEscopo *cauda_log = NULL;  

static Escopo *escopo_novo(const char *desc) {
    Escopo *e = calloc(1, sizeof(Escopo));
    if (!e) { perror("symtab: calloc Escopo"); exit(EXIT_FAILURE); }
    strncpy(e->desc, desc, sizeof(e->desc) - 1);

    LogEscopo *entrada = calloc(1, sizeof(LogEscopo));
    if (!entrada) { perror("symtab: calloc LogEscopo"); exit(EXIT_FAILURE); }
    entrada->escopo = e;
    if (!cauda_log) { 
        cabeca_log = cauda_log = entrada; 
    }
    else { 
        cauda_log->prox = entrada; cauda_log = entrada; 
    }

    return e;
}

void ts_iniciar(void) {
    atual      = NULL;
    cabeca_log = cauda_log = NULL;
    ts_entrar_escopo("global");
}

void ts_destruir(void) {
    LogEscopo *entrada = cabeca_log;
    while (entrada) {
        Simbolo *sim = entrada->escopo->simbolos;
        while (sim) {
            Simbolo *prox = sim->prox;
            free(sim);
            sim = prox;
        }
        free(entrada->escopo);
        LogEscopo *prox_e = entrada->prox;
        free(entrada);
        entrada = prox_e;
    }
    atual      = NULL;
    cabeca_log = cauda_log = NULL;
}

void ts_entrar_escopo(const char *desc) {
    Escopo *e = escopo_novo(desc);
    e->pai = atual;
    atual  = e;
}

void ts_sair_escopo(void) {
    if (!atual) return;
    atual = atual->pai;
}

const char *ts_escopo_atual(void) {
    return atual ? atual->desc : "(nenhum)";
}

void ts_desc_bloco(char *buf, size_t bufsz) {
    if (!atual) { snprintf(buf, bufsz, "block#?"); return; }

    Escopo *raiz = atual;
    while (raiz && strstr(raiz->desc, ".locals") == NULL)
        raiz = raiz->pai;

    if (!raiz) 
        raiz = atual;

    raiz->cnt_blocos++;

    char base[256];
    strncpy(base, raiz->desc, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    char *ponto = strstr(base, ".locals");
    if (ponto) *ponto = '\0';

    snprintf(buf, bufsz, "%s.block#%d", base, raiz->cnt_blocos);
}

int ts_inserir(const char *nome, Categoria cat, Tipo tipo, int extra) {
    if (!atual) return -1;

    for (Simbolo *s = atual->simbolos; s; s = s->prox) {
        if (strcmp(s->nome, nome) == 0) return -1;
    }

    Simbolo *sim = calloc(1, sizeof(Simbolo));
    if (!sim) { perror("symtab: calloc Simbolo"); exit(EXIT_FAILURE); }
    strncpy(sim->nome, nome, sizeof(sim->nome) - 1);
    sim->cat   = cat;
    sim->tipo  = tipo;
    sim->extra = extra;

    if (!atual->simbolos) {
        atual->simbolos = sim;
    } else {
        Simbolo *cauda = atual->simbolos;
        while (cauda->prox) cauda = cauda->prox;
        cauda->prox = sim;
    }
    return 0;
}

Simbolo *ts_buscar(const char *nome) {
    for (Escopo *esc = atual; esc; esc = esc->pai) {
        for (Simbolo *s = esc->simbolos; s; s = s->prox) {
            if (strcmp(s->nome, nome) == 0) return s;
        }
    }
    return NULL;
}

static const char *cat_para_str(Categoria c) {
    switch (c) {
        case CAT_VAR:    return "var";
        case CAT_VETOR:  return "vetor";
        case CAT_PROC:   return "proc";
        case CAT_FUNCAO: return "funcao";
        case CAT_PARAM:  return "param";
        default:         return "?";
    }
}

static const char *tipo_para_str(Tipo t) {
    switch (t) {
        case TIPO_INT:    return "int";
        case TIPO_BOOL:   return "bool";
        case TIPO_CHAR:   return "char";
        case TIPO_NENHUM: return "nenhum";
        default:          return "?";
    }
}

void ts_dump(FILE *fp) {
    for (LogEscopo *entrada = cabeca_log; entrada; entrada = entrada->prox) {
        Escopo *esc = entrada->escopo;
        for (Simbolo *s = esc->simbolos; s; s = s->prox) {
            fprintf(fp, "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s  extra=%d\n",
                    esc->desc, s->nome,
                    cat_para_str(s->cat), tipo_para_str(s->tipo), s->extra);
        }
    }
}
