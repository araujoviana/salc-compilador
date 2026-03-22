#include "lex.h"
#include "diag.h" // Invoca diag pra gerar erros
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define APPEND_LEXEME(lex, len, c)                                             \
  (lex[len++] = (char)c) // Adiciona char ao lexema

typedef struct {
  const char *text;
  Category category;
} KeywordMap;

KeywordMap keyword_map[] = {
    {"bool", sBOOL},       {"char", sCHAR},
    {"do", sDO},           {"else", sELSE},
    {"end", sEND},         {"false", sBOOL}, // Tratado como valor booleano
    {"fn", sFN},           {"for", sFOR},
    {"globals", sGLOBALS}, {"if", sIF},
    {"int", sINT},         {"locals", sLOCALS},
    {"loop", sLOOP},       {"match", sMATCH},
    {"module", sMODULE},   {"otherwise", sOTHERWISE},
    {"print", sPRINT},     {"proc", sPROC},
    {"ret", sRETURN},      {"scan", sSCAN},
    {"start", sSTART},     {"step", sSTEP},
    {"to", sTO},           {"true", sBOOL}, // Tratado como valor booleano
    {"until", sUNTIL},     {"while", sWHILE},
    {"main", sMAIN} // Procedimento principal
};

#define TOTAL_KEYWORDS (sizeof(keyword_map) / sizeof(KeywordMap))

static Category get_keyword(char *lexeme) {
  for (int i = 0; i < TOTAL_KEYWORDS; i++) {
    if (strcmp(lexeme, keyword_map[i].text) == 0) {
      return keyword_map[i].category;
    }
  }
  return sIDENTIF;
}

typedef enum {
  START,
  IDENT,
  STRING,
  CHAR_START,
  CHAR_ESC, // Char tipo \n
  CHAR_MID,
  NUMBER,
  AMBIG_COMMENT, // Pode ser line ou block comment
  LINE_COMMENT,
  BLOCK_COMMENT,
  BLOCK_COMMENT_ENDING, // Entre } e @
  COLON,
  DOT,
  EQUAL,
  GREATER,
  LOWER,
  OR,
  AND
} State;

static Token make_token(Category cat, const char *lex, int line, int pos) {
  Token t = {0};
  t.category = cat;
  strncpy(t.lexema, lex, sizeof(t.lexema) - 1);
  t.line = line;
  t.position = pos;
  return t;
}

Token lex_next(FILE *file_ptr, int *line_cnt) {
  Token token = {0}; // Token retornada completa

  char lexeme[LEX_LENGTH]; // Lexema acumulado
  size_t l_len = 0;        // Tamanho do lexema

  int c; // char lido

  State state = START; // Estado inicial

  while ((c = fgetc(file_ptr)) != EOF) {
    switch (state) {
      // Início
    case START:
      // Ignora espaços e conta linhas
      if (isspace(c)) {
        if (c == '\n') {
          (*line_cnt)++;
        }
        continue;
      }
      // String
      if (c == '\"') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = STRING;
        break;
      }
      // Char
      else if (c == '\'') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_START;
        break;
      }
      // Número
      else if (isdigit(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        state = NUMBER;
        break;
      }
      // Ou
      else if (c == 'v') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = OR;
        break;
      }
      // E
      else if (c == '^') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = AND;
        break;
      }
      // Identificador (variável ou keyword)
      else if (isalpha(c) || c == '_') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = IDENT;
        break;
      }
      // Comentário ambíguo
      else if (c == '@') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = AMBIG_COMMENT;
        break;
      }
      // Dois pontos
      else if (c == ':') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = COLON;
        break;
      }
      // Ponto
      else if (c == '.') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = DOT;
        break;
      }
      // Igual
      else if (c == '=') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = EQUAL;
        break;
      }
      // Maior que
      else if (c == '>') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = GREATER;
        break;
      }
      // Menor que
      else if (c == '<') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = LOWER;
        break;
      }

      // -- Não precisam de estado separado por serem únicos

      // Soma
      else if (c == '+') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sSOMA, lexeme, *line_cnt, 0);
      }
      // Subtração
      else if (c == '-') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sSUBRAT, lexeme, *line_cnt, 0);
      }
      // Multiplicação
      else if (c == '*') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sMULT, lexeme, *line_cnt, 0);
      }
      // Divisão
      else if (c == '/') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sDIV, lexeme, *line_cnt, 0);
      }
      // Negação
      else if (c == '~') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sNEG, lexeme, *line_cnt, 0);
      }
      // Ponto e vírgula
      else if (c == ';') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sPTO_VIRG, lexeme, *line_cnt, 0);
      }
      // Vírgula
      else if (c == ',') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sVIRGULA, lexeme, *line_cnt, 0);
      }
      // Abre parenteses
      else if (c == '(') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sABRE_PARENT, lexeme, *line_cnt, 0);
      }
      // Fecha parenteses
      else if (c == ')') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sFECHA_PARENT, lexeme, *line_cnt, 0);
      }
      // Abre colchetes
      else if (c == '[') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sABRE_COLCH, lexeme, *line_cnt, 0);
      }
      // Fecha colchetes
      else if (c == ']') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sFECHA_COLCH, lexeme, *line_cnt, 0);
      }
      // REVIEW Caractere desconhecido!
      else {
        char found[2] = {(char)c, '\0'};
        diag_error("caractere valido", found, *line_cnt);
        continue;
      }
    case STRING:
      // String finalizada
      if (c == '\"') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sSTRING, lexeme, *line_cnt, 0);
      } else {
        APPEND_LEXEME(lexeme, l_len, c);
        break;
      }
    case CHAR_START:
      // Char vazio
      if (c == '\'') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sCTECHAR, lexeme, *line_cnt, 0);
      }
      // Char escapado tipo \n
      else if (c == '\\') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_ESC;
        break;
      } else {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_MID;
        break;
      }
    case CHAR_ESC:
      if (isalpha(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_MID;
        break;
      } else {
        char found[2] = {(char)c, '\0'};
        diag_error("caractere de escape valido (ex: \\n)", found, *line_cnt);
        state = START;
        break;
      }
    case CHAR_MID:
      if (c == '\'') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sCTECHAR, lexeme, *line_cnt, 0);
      }
      // Char não terminou com '
      else {
        char found[2] = {(char)c, '\0'};
        diag_error("'\''", found, *line_cnt);           // Esperava aspa simples
        return make_token(sEND, "ERROR", *line_cnt, 0); // Aborta tokenização
      }
    case NUMBER:
      if (isdigit(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        break;
      } else {
        ungetc(c, file_ptr);
        lexeme[l_len] = '\0';
        return make_token(sCTEINT, lexeme, *line_cnt, 0);
      }
    case IDENT:
      // Números são valídos dentro de identificadores
      if (isalpha(c) || c == '_' || isdigit(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        break;
      } else {
        ungetc(c, file_ptr);
        Category ident_category = get_keyword(lexeme); // Variável ou keyword
        lexeme[l_len] = '\0';
        return make_token(ident_category, lexeme, *line_cnt, 0);
      }
    case AMBIG_COMMENT:
      // Comentário de bloco
      if (c == '{') {
        state = BLOCK_COMMENT;
      }
      // Comentário de linha
      else {
        ungetc(c, file_ptr);
        state = LINE_COMMENT;
      }
      break;
    case LINE_COMMENT:
      // Comentário terminou
      if (c == '\n') {
        (*line_cnt)++;
        l_len = 0; // Prepara o lexema pra próxima
        state = START;
      }
      break;
    case BLOCK_COMMENT:
      if (c == '\n') {
        (*line_cnt)++;
      } else if (c == '}') {
        state = BLOCK_COMMENT_ENDING;
      }
      break;
    case BLOCK_COMMENT_ENDING:
      if (c == '@') {
        state = START;
        l_len = 0;
      } else {
        state = BLOCK_COMMENT;
      }
      break;
    case COLON:
      if (c == '=') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sATRIB, lexeme, *line_cnt, 0);
      } else {
        ungetc(c, file_ptr);
        lexeme[l_len] = '\0';
        return make_token(sDOIS_PTOS, lexeme, *line_cnt, 0);
      }
    case DOT:
      if (c == '.') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sPTOPTO, lexeme, *line_cnt, 0);
      } else {
        char found[2] = {(char)c, '\0'};
        diag_error("'.'", found, *line_cnt);
        ungetc(c, file_ptr);
        state = START;
        l_len = 0;
        break;
      }
    case EQUAL:
      if (c == '>') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sIMPLIC, lexeme, *line_cnt, 0);
      } else {
        ungetc(c, file_ptr);
        lexeme[l_len] = '\0';
        return make_token(sIGUAL, lexeme, *line_cnt, 0);
      }
    case GREATER:
      if (c == '=') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sMAIORIG, lexeme, *line_cnt, 0);
      } else {
        ungetc(c, file_ptr);
        lexeme[l_len] = '\0';
        return make_token(sMAIOR, lexeme, *line_cnt, 0);
      }
    case LOWER:
      if (c == '=') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sMENORIG, lexeme, *line_cnt, 0);
      } else if (c == '>') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sDIFERENTE, lexeme, *line_cnt, 0);
      } else {
        ungetc(c, file_ptr);
        lexeme[l_len] = '\0';
        return make_token(sMENOR, lexeme, *line_cnt, 0);
      }
    case OR:
      if (isalpha(c) || isdigit(c) || c == '_') {
        ungetc(c, file_ptr);
        state = IDENT;
        break;
      }
      ungetc(c, file_ptr);
      lexeme[l_len] = '\0';
      return make_token(sOR, lexeme, *line_cnt, 0);
    case AND:
      if (isalpha(c) || isdigit(c) || c == '_') {
        ungetc(c, file_ptr);
        state = IDENT;
        break;
      }
      ungetc(c, file_ptr);
      lexeme[l_len] = '\0';
      return make_token(sAND, lexeme, *line_cnt, 0);
    }
  }
  return make_token(sEOF, "EOF", *line_cnt, 0);
}
