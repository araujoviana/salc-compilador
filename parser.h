#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "lex.h"

int parser_parse(FILE *source);

static Token token;
static Token next_token;
static FILE *src;
static int line_cnt = 1;

static void parse_ini(void);
static void parse_glob(void);
static void parse_subs(void);
static void parse_decls(void);
static void parse_decl_item(void);
static void parse_tpo(void);
static void parse_func(void);
static void parse_proc(void);
static void parse_princ(void);
static void parse_locals_opt(void);
static void parse_param(void);
static void parse_bco(void);
static void parse_cmd(void);
static void parse_out(void);
static void parse_inp(void);
static void parse_if(void);
static void parse_mat(void);
static void parse_wlist(void);
static void parse_when(void);
static void parse_otherwise(void);
static void parse_wcnd(void);
static void parse_witem(void);
static void parse_wint(void);
static void parse_fr(void);
static void parse_loop(void);
static void parse_ret(void);
static void parse_atr(void);
static void parse_call(void);
static void parse_vec(void);
static void parse_id(void);
static void parse_elem(void);
static void parse_expr(void);
static void parse_exlog(void);
static void parse_exrel(void);
static void parse_exari(void);
static void parse_exarp(void);
static void parse_fact(void);
static void parse_primary(void);
static void parse_litl(void);
#endif
