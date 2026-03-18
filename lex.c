#include "lex.h"
#include "diag.h" // Invoca diag pra gerar erros
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define LEX_LENGTH 1024

typedef enum {
  START,
  // TODO
} State;

// BOTAR NO HEADER FILE QUANDO PRONTO
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
  sFECHA_COLCH
} Category;

// BOTAR NO HEADER FILE QUANDO PRONTO
/// Flags das opções da linha de comando
typedef struct {
  char lexema[LEX_LENGTH]; // Texto original do token
  int line;                // Linha do token
  int position;            // Não sei se precisa
} Token;

#define APPEND_LEXEME(lex, len, c)                                             \
  (lex[len++] = (char)c) // Adiciona char ao lexema

static Token make_token(Category cat, const char *lex, int line, int pos) {
  Token t = {0};
  t.Category = cat;
  strncpy(t.lexema, lex, sizeof(t.lexema) - 1);
  t.line = line;
  t.position = pos;
  return t;
}

Token lex_next(FILE *file_ptr, int *line_cnt) {
  int col = 0; // TODO implementar contagem de coluna reiniciando \n

  Token token = {0}; // Token retornada completa

  char lexeme[1024]; // Lexema acumulado
  size_t l_len = 0;  // Tamanho do lexema

  int c; // char lido
  State state = START;

  while ((c = fgetc(file_ptr)) != EOF) {
    APPEND_LEXEME(lexeme, l_len, c);

    // Separadores de 1 char
    switch (c) {
    case ';': // Ponto e virgula
      return make_token(sPTO_VIRG, ";", *line_cnt, col);
    case ':': // Dois pontos
      return make_token(sDOIS_PTOS, ":", *line_cnt, col);
    case ',': // Virgula
      return make_token(sVIRGULA, ",", *line_cnt, col);
    case '(': // Abre parenteses
      return make_token(sABRE_PARENT, "(", *line_cnt, col);
    case ')': // Fecha parenteses
      return make_token(sFECHA_PARENT, ")", *line_cnt, col);
    case '[': // Abre colchete
      return make_token(sABRE_COLCH, "[", *line_cnt, col);
    case ']': // Fecha colchete
      return make_token(sFECHA_COLCH, "]", *line_cnt, col);
    }

    // Operadores (únicos) de 1 char
    switch (c) {
    case '+': // Soma
      return make_token(sSOMA, "+", *line_cnt, col);
    case '-': // Subtração
      return make_token(sSUBRAT, "-", *line_cnt, col);
    case '*':
      return make_token(sMULT, "*", *line_cnt, col);
    case '/':
      return make_token(sDIV, "/", *line_cnt, col);
    case '^':
      return make_token(sAND, "^", *line_cnt, col);
    case '~':
      return make_token(sNEG, "~", *line_cnt, col);
    }

    // Operadores ambíguos de 1 ou mais chars
    switch (c) {
    case '=':
      c = fgetc(file_ptr);
      if (c == '>') {
        return make_token(sIMPLIC, "=>", *line_cnt, col);
      } else {
        return make_token(sIGUAL, "=", *line_cnt, col);
      }
    case '>':
      c = fgetc(file_ptr);
      if (c == '=') {
        return make_token(sMAIORIG, ">=", *line_cnt, col);
      } else {
        return make_token(sMAIOR, ">", *line_cnt, col);
      }
    case '<':
      c = fgetc(file_ptr);
      if (c == '=') {
        return make_token(sMENORIG, "<=", *line_cnt, col);
      } else if (c == '>') {
        return make_token(sDIFERENTE, "<>", *line_cnt, col);
      } else {
        return make_token(sMENOR, "<", *line_cnt, col);
      }
    }

    // Operadores com 2 chars
    switch (c) {
    case ':':
      c = fgetc(file_ptr);
      if (c == '=') {
        return make_token(sATRIB, ":=", *line_cnt, col);
      } else {
        diag_error("=", c, line_cnt); // FIXME segundo argumento
        return NULL;
      }
    case '.':
      c = fgetc(file_ptr);
      if (c == '.') {
        return make_token(sPTOPTO, "..", *line_cnt, col);
      } else {
        diag_error(".", c, line_cnt); // FIXME segundo argumento
        return NULL;
      }
    }

    break;
  }
}
