/*
 * Projeto SALc - Fase 1
 * Arquivo: lex.c
 * Integrantes:
 * - Matheus Gabriel Viana Araujo - 10420444
 * - Luis Fernando de Mesquita Pereira - 10410686
 */

#include "lex.h"
#include "diag.h" // Usa o modulo de diagnostico para avisar erros.
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define APPEND_LEXEME(lex, len, c)                                             \
  (lex[len++] = (char)c) // Coloca mais um caractere no lexema atual.

typedef struct {
  const char *text;
  Category category;
} KeywordMap;

static const KeywordMap keyword_map[] = {
    {"bool", sBOOL},       {"char", sCHAR},
    {"do", sDO},           {"else", sELSE},
    {"end", sEND},         {"false", sBOOL}, // Literal booleano.
    {"fn", sFN},           {"for", sFOR},
    {"globals", sGLOBALS}, {"if", sIF},
    {"int", sINT},         {"locals", sLOCALS},
    {"loop", sLOOP},       {"match", sMATCH},
    {"module", sMODULE},   {"otherwise", sOTHERWISE},
    {"print", sPRINT},     {"proc", sPROC},
    {"ret", sRETURN},      {"scan", sSCAN},
    {"start", sSTART},     {"step", sSTEP},
    {"to", sTO},           {"true", sBOOL}, // Literal booleano.
    {"until", sUNTIL},     {"when", sWHEN},
    {"while", sWHILE},     {"main", sMAIN} // Nome reservado da principal.
};

#define TOTAL_KEYWORDS (sizeof(keyword_map) / sizeof(KeywordMap))

static Category get_keyword(const char *lexeme) {
  for (size_t i = 0; i < TOTAL_KEYWORDS; i++) {
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
  CHAR_ESC, // Caractere com escape, como \n.
  CHAR_MID,
  NUMBER,
  AMBIG_COMMENT, // Pode virar comentario de linha ou de bloco.
  LINE_COMMENT,
  BLOCK_COMMENT,
  BLOCK_COMMENT_ENDING, // Leu o } e espera fechar com @.
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

static Token make_error_token(const char *expected, const char *found,
                              int line) {
  diag_error(expected, found, line);
  return make_token(sERROR, found, line, 0);
}

static bool is_char_escape(int c) {
  return c == 'n' || c == 't' || c == 'r' || c == '0' || c == '\\' ||
         c == '\'' || c == '"';
}

Token lex_next(FILE *file_ptr, int *line_cnt) {
  char lexeme[LEX_LENGTH]; // Lexema que esta sendo montado.
  size_t l_len = 0;        // Tamanho atual do lexema.

  int c; // Caractere lido do arquivo.

  State state = START; // Estado atual do automato.

  while ((c = fgetc(file_ptr)) != EOF) {
    switch (state) {
      // Inicio da leitura de um novo token.
    case START:
      // Ignora espacos e atualiza a linha.
      if (isspace(c)) {
        if (c == '\n') {
          (*line_cnt)++;
        }
        continue;
      }
      // Literal de texto.
      if (c == '\"') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = STRING;
        break;
      }
      // Caractere.
      else if (c == '\'') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_START;
        break;
      }
      // Numero inteiro.
      else if (isdigit(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        state = NUMBER;
        break;
      }
      // Operador logico ou.
      else if (c == 'v') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = OR;
        break;
      }
      // Operador logico e.
      else if (c == '^') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = AND;
        break;
      }
      // Identificador ou palavra reservada.
      else if (isalpha(c) || c == '_') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = IDENT;
        break;
      }
      // Comentario: pode ser de linha ou de bloco.
      else if (c == '@') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = AMBIG_COMMENT;
        break;
      }
      // Dois pontos.
      else if (c == ':') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = COLON;
        break;
      }
      // Ponto.
      else if (c == '.') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = DOT;
        break;
      }
      // Igual.
      else if (c == '=') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = EQUAL;
        break;
      }
      // Maior que.
      else if (c == '>') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = GREATER;
        break;
      }
      // Menor que.
      else if (c == '<') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = LOWER;
        break;
      }

      // Os simbolos abaixo ja formam token sozinhos.

      // Soma.
      else if (c == '+') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sSOMA, lexeme, *line_cnt, 0);
      }
      // Subtracao.
      else if (c == '-') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sSUBRAT, lexeme, *line_cnt, 0);
      }
      // Multiplicacao.
      else if (c == '*') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sMULT, lexeme, *line_cnt, 0);
      }
      // Divisao.
      else if (c == '/') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sDIV, lexeme, *line_cnt, 0);
      }
      // Negacao.
      else if (c == '~') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sNEG, lexeme, *line_cnt, 0);
      }
      // Ponto e virgula.
      else if (c == ';') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sPTO_VIRG, lexeme, *line_cnt, 0);
      }

      // Virgula.
      else if (c == ',') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sVIRGULA, lexeme, *line_cnt, 0);
      }
      // Abre parenteses.
      else if (c == '(') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sABRE_PARENT, lexeme, *line_cnt, 0);
      }
      // Fecha parenteses.
      else if (c == ')') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sFECHA_PARENT, lexeme, *line_cnt, 0);
      }
      // Abre colchetes.
      else if (c == '[') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sABRE_COLCH, lexeme, *line_cnt, 0);
      }
      // Fecha colchetes.
      else if (c == ']') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sFECHA_COLCH, lexeme, *line_cnt, 0);
      }
      // Caractere que nao pertence a linguagem.
      else {
        char found[2] = {(char)c, '\0'};
        return make_error_token("caractere valido", found, *line_cnt);
      }
    case STRING:
      // Fechou o texto.
      if (c == '\"') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sSTRING, lexeme, *line_cnt, 0);
      } else if (c == '\n') {
        return make_error_token("fechamento de string (\")",
                                "quebra de linha", *line_cnt);
      } else {
        APPEND_LEXEME(lexeme, l_len, c);
        break;
      }
    case CHAR_START:
      // Caractere vazio nao vale em SAL.
      if (c == '\'') {
        return make_error_token("caractere entre aspas simples", "''",
                                *line_cnt);
      }
      // Caractere com escape.
      else if (c == '\\') {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_ESC;
        break;
      } else if (c == '\n') {
        return make_error_token("caractere entre aspas simples",
                                "quebra de linha", *line_cnt);
      } else {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_MID;
        break;
      }
    case CHAR_ESC:
      if (is_char_escape(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        state = CHAR_MID;
        break;
      } else {
        char found[2] = {(char)c, '\0'};
        return make_error_token("caractere de escape valido", found,
                                *line_cnt);
      }
    case CHAR_MID:
      if (c == '\'') {
        APPEND_LEXEME(lexeme, l_len, c);
        lexeme[l_len] = '\0';
        return make_token(sCTECHAR, lexeme, *line_cnt, 0);
      }
      // Se vier outro simbolo, faltou fechar o caractere.
      else {
        char found[2] = {(char)c, '\0'};
        return make_error_token("'", found, *line_cnt);
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
      // Depois da primeira letra, pode ter letra, numero ou sublinhado.
      if (isalpha(c) || c == '_' || isdigit(c)) {
        APPEND_LEXEME(lexeme, l_len, c);
        break;
      } else {
        ungetc(c, file_ptr);
        lexeme[l_len] = '\0';
        Category ident_category = get_keyword(lexeme); // Identificador ou reservada.
        return make_token(ident_category, lexeme, *line_cnt, 0);
      }
    case AMBIG_COMMENT:
      // Comentario de bloco.
      if (c == '{') {
        state = BLOCK_COMMENT;
      }
      // Comentario de linha.
      else {
        ungetc(c, file_ptr);
        state = LINE_COMMENT;
      }
      break;
    case LINE_COMMENT:
      // O comentario de linha acaba quando chega o \n.
      if (c == '\n') {
        (*line_cnt)++;
        l_len = 0; // Limpa o lexema para o proximo token.
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
        ungetc(c, file_ptr);
        return make_error_token("..", ".", *line_cnt);
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

  lexeme[l_len] = '\0';

  switch (state) {
  case NUMBER:
    return make_token(sCTEINT, lexeme, *line_cnt, 0);
  case IDENT:
    return make_token(get_keyword(lexeme), lexeme, *line_cnt, 0);
  case OR:
    return make_token(sOR, lexeme, *line_cnt, 0);
  case AND:
    return make_token(sAND, lexeme, *line_cnt, 0);
  case COLON:
    return make_token(sDOIS_PTOS, lexeme, *line_cnt, 0);
  case EQUAL:
    return make_token(sIGUAL, lexeme, *line_cnt, 0);
  case GREATER:
    return make_token(sMAIOR, lexeme, *line_cnt, 0);
  case LOWER:
    return make_token(sMENOR, lexeme, *line_cnt, 0);
  case STRING:
    return make_error_token("fechamento de string (\")", "EOF", *line_cnt);
  case CHAR_START:
  case CHAR_ESC:
  case CHAR_MID:
    return make_error_token("'", "EOF", *line_cnt);
  case BLOCK_COMMENT:
  case BLOCK_COMMENT_ENDING:
    return make_error_token("}@", "EOF", *line_cnt);
  case DOT:
    return make_error_token("..", "EOF", *line_cnt);
  case AMBIG_COMMENT:
  case LINE_COMMENT:
  case START:
  default:
    break;
  }

  return make_token(sEOF, "EOF", *line_cnt, 0);
}
