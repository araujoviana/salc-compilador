#ifndef SYMTAB_H
#define SYMTAB_H

#include "lex.h"

typedef enum {
    CAT_VAR,     // variável simples   
    CAT_VETOR,   // variável vetor     
    CAT_PROC,    // procedimento       
    CAT_FUNCAO,  // função             
    CAT_PARAM    // parâmetro          
} Categoria;

typedef enum {
    TIPO_INT,
    TIPO_BOOL,
    TIPO_CHAR,
    TIPO_NENHUM  
} Tipo;

typedef struct Simbolo {
    char           nome[LEX_LENGTH];
    Categoria      cat;
    Tipo           tipo;
    int            extra;        
    struct Simbolo *prox;
} Simbolo;

typedef struct Escopo {
    char           desc[256];  
    Simbolo       *simbolos;   
    struct Escopo *pai;         
    int            cnt_blocos;  
} Escopo;

// Inicializa a TS e cria o escopo global 
void ts_iniciar(void);
void ts_destruir(void);
void ts_entrar_escopo(const char *desc);
void ts_sair_escopo(void);
const char   *ts_escopo_atual(void);
void ts_desc_bloco(char *buf, size_t bufsz);
int ts_inserir(const char *nome, Categoria cat, Tipo tipo, int extra);
Simbolo *ts_buscar(const char *nome);
void ts_dump(FILE *fp);

#endif 
