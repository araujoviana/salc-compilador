/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#ifndef GERADOR_H
#define GERADOR_H

#include <stdio.h>

/* Inicializa o gerador com o arquivo de saida. */
void gen_init(FILE *fp);

/* Emite uma instrucao MEPA formatada.
 * rotulo    - rotulo da instrucao ("L1") ou NULL
 * mnemonico - instrucao ("SOMA", "CRVL", ...)
 * parametro1 - primeiro parametro ou NULL
 * parametro2 - segundo parametro ou NULL (separado por virgula, sem espaco)
 *
 * Exemplos:
 *   gera_instr_mepa(NULL, "INPP", NULL, NULL)  -> "     INPP\n"
 *   gera_instr_mepa(NULL, "AMEM", "3", NULL)   -> "     AMEM 3\n"
 *   gera_instr_mepa(NULL, "CRVL", "0", "2")    -> "     CRVL 0,2\n"
 *   gera_instr_mepa("L1", "NADA", NULL, NULL)  -> "L1:  NADA\n"
 */
void gera_instr_mepa(char *rotulo, char *mnemonico,
                     char *parametro1, char *parametro2);

/* Retorna um novo rotulo unico ("L1", "L2", ...).
 * O ponteiro retornado e valido ate a proxima chamada com o mesmo slot do pool.
 * O pool suporta 512 rotulos simultaneos — suficiente para qualquer programa SAL.
 */
char *novo_rotulo(void);

/* Suspende / reabilita a emissao MEPA (uso interno do parser). */
void gen_suspend(void);
void gen_resume(void);

#endif /* GERADOR_H */
