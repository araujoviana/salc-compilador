#ifndef DIAG_H
#define DIAG_H

int diag_error(const char *token_expected, const char *token_found, int line);

int diag_info(const char *msg);

#endif
