/*
 * Matheus Gabriel Viana Araujo - 10420444
 * Luis Fernando de Mesquita Pereira - 10410686
 */

#ifndef DIAG_H
#define DIAG_H

int diag_error(const char *token_expected, const char *token_found, int line);

int diag_info(const char *msg);

#endif
