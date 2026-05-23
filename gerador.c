/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#include "gerador.h"

#include <stdio.h>

#define LABEL_POOL_SIZE 512
#define LABEL_BUF_SIZE  16

static FILE *g_out          = NULL;
static FILE *g_saved_out    = NULL;
static int   g_label_counter = 0;
static int   g_pool_index    = 0;
static char  g_label_pool[LABEL_POOL_SIZE][LABEL_BUF_SIZE];

void gen_init(FILE *fp) {
    g_out          = fp;
    g_label_counter = 0;
    g_pool_index    = 0;
}

void gera_instr_mepa(char *rotulo, char *mnemonico,
                     char *parametro1, char *parametro2) {
    if (!g_out || !mnemonico) return;

    if (rotulo) {
        fprintf(g_out, "%s:  %s", rotulo, mnemonico);
    } else {
        fprintf(g_out, "     %s", mnemonico);
    }

    if (parametro1) {
        if (parametro2) {
            fprintf(g_out, " %s,%s", parametro1, parametro2);
        } else {
            fprintf(g_out, " %s", parametro1);
        }
    }

    fprintf(g_out, "\n");
}

char *novo_rotulo(void) {
    int slot = g_pool_index % LABEL_POOL_SIZE;
    g_pool_index++;
    snprintf(g_label_pool[slot], LABEL_BUF_SIZE, "L%d", ++g_label_counter);
    return g_label_pool[slot];
}

void gen_suspend(void) {
    g_saved_out = g_out;
    g_out       = NULL;
}

void gen_resume(void) {
    g_out = g_saved_out;
}
