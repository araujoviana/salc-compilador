# salc-compilador

Compilador da primeira fase da linguagem SAL (Simple Academic Language), implementado em C para a disciplina de Compiladores.

- Matheus Gabriel Viana Araujo - 10420444
- Luis Fernando de Mesquita Pereira - 10410686

## Escopo desta entrega

Esta versao cobre a frente do compilador pedida nas orientacoes da fase 1:

- analise lexica da linguagem SAL;
- analise sintatica por descida recursiva;
- tabela de simbolos com controle de escopo;
- geracao opcional de logs de tokens, tabela de simbolos e rastreamento da analise.

Geracao de codigo MEPA e analise semantica completa ficam fora do escopo atual.

## Estrutura do projeto

- `main.c`: orquestracao da execucao e integracao dos modulos.
- `opt.c` / `opt.h`: leitura das opcoes de linha de comando.
- `lex.c` / `lex.h`: analisador lexico da linguagem SAL.
- `parser.c` / `parser.h`: parser recursivo descendente.
- `symtab.c` / `symtab.h`: tabela de simbolos com escopos aninhados.
- `diag.c` / `diag.h`: diagnosticos de erro e informacoes de trace.
- `log.c` / `log.h`: geracao dos arquivos `.tk`, `.ts` e `.trc`.
- `guidance/`: especificacao, orientacoes e materiais da disciplina.
- `tests/`: casos simples de regressao para execucao local.

## Compilacao

```bash
make
```

O comando gera o executavel `salc`.

## Uso

```bash
./salc <arquivo.sal> [--tokens] [--symtab] [--trace]
```

Exemplo:

```bash
./salc teste.sal --tokens --symtab --trace
```

## Logs gerados

- `arquivo.tk`: lista de tokens reconhecidos pelo lexico.
- `arquivo.ts`: tabela de simbolos consolidada por escopo.
- `arquivo.trc`: rastreamento simples do progresso do parser.

## Testes rapidos

```bash
make test
```

O alvo `test` executa casos validos e invalidos para verificar regressao em pontos importantes do compilador.

## Referencia da linguagem

As principais referencias usadas no projeto estao em `guidance/`:

- `guidance/orientacoes.txt`
- `guidance/SAL - Especificação da Linguagem.pdf`
- materiais de analise lexica e sintatica fornecidos na disciplina
