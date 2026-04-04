# salc-compilador

Compilador da primeira fase da linguagem SAL.

- Matheus Gabriel Viana Araújo - 10420444
- Luis Fernando de Mesquita Pereira - 10410686

## Escopo desta entrega

Esta versão cobre a frente do compilador pedida nas orientações da fase 1:

- análise léxica da linguagem SAL;
- análise sintática por descida recursiva;
- tabela de símbolos com controle de escopo;
- geração opcional de logs de tokens, tabela de símbolos e rastreamento da análise.

Geração de código MEPA e análise semântica completa ficam fora do escopo atual.

## Estrutura do projeto

- `main.c`: orquestração da execução e integração dos módulos.
- `opt.c` / `opt.h`: leitura das opções de linha de comando.
- `lex.c` / `lex.h`: analisador léxico da linguagem SAL.
- `parser.c` / `parser.h`: parser recursivo descendente.
- `symtab.c` / `symtab.h`: tabela de símbolos com escopos aninhados.
- `diag.c` / `diag.h`: diagnósticos de erro e informações de trace.
- `log.c` / `log.h`: geração dos arquivos `.tk`, `.ts` e `.trc`.
- `guidance/`: especificação, orientações e materiais da disciplina.
- `tests/`: casos simples de regressão para execução local.

## Compilação

```bash
make
```

O comando gera o executável `salc`.

## Uso

```bash
./salc <arquivo.sal> [--tokens] [--symtab] [--trace]
```

Exemplo:

```bash
./salc teste.sal --tokens --symtab --trace
```

## Logs gerados

- `arquivo.tk`: lista de tokens reconhecidos pelo léxico.
- `arquivo.ts`: tabela de símbolos consolidada por escopo.
- `arquivo.trc`: rastreamento simples do progresso do parser.
