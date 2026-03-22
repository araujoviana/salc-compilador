#ifndef LEX_H
#define LEX_H

#include <stdio.h>

#define LEX_LENGTH 1024

typedef enum {
  sIDENTIF,
  sCTEINT,
  sCTECHAR,
  sSTRING,
  sMODULE,
  sGLOBALS,
  sLOCALS,
  sSTART,
  sEND,
  sINT,
  sBOOL,
  sCHAR,
  sFN,
  sPROC,
  sMAIN,
  sRETURN,
  sPRINT,
  sSCAN,
  sIF,
  sELSE,
  sMATCH,
  sWHEN,
  sOTHERWISE,
  sFOR,
  sSTEP,
  sTO,
  sLOOP,
  sWHILE,
  sUNTIL,
  sDO,
  sATRIB,
  sIMPLIC,
  sPTOPTO,
  sSOMA,
  sSUBRAT,
  sMULT,
  sDIV,
  sIGUAL,
  sDIFERENTE,
  sMAIOR,
  sMAIORIG,
  sMENOR,
  sMENORIG,
  sOR,
  sAND,
  sNEG,
  sPTO_VIRG,
  sDOIS_PTOS,
  sVIRGULA,
  sABRE_PARENT,
  sFECHA_PARENT,
  sABRE_COLCH,
  sFECHA_COLCH,
  sEOF // Adicionado para idenficar final do arquivo
} Category;

typedef struct {
  Category category;
  char lexema[LEX_LENGTH];
  int line;
  int position;
} Token;

Token lex_next(FILE *file_ptr, int *line_cnt);

#endif
