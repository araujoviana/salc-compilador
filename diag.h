#ifndef DIAG_H
#define DIAG_H

int diag_error(char *token_expected, char *token_found, int line);

int diag_info(char *msg);

#endif
