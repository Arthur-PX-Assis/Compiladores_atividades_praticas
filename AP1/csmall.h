#ifndef CSMALL_H
#define CSMALL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAM_LEXEMA 128

typedef enum {
    TOK_INT, TOK_FLOAT, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_PRINT,
    TOK_IDENTIFIER, TOK_INT_CONST, TOK_FLOAT_CONST,
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV,
    TOK_ASSIGN, TOK_LT, TOK_LE, TOK_GT, TOK_GE, TOK_EQ, TOK_NE,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON,
    TOK_EOF, TOK_INVALID
} TokenType;

typedef struct {
    TokenType tipo;
    char lexema[TAM_LEXEMA];
    int linha;
} Token;

typedef struct {
    FILE *arquivo;
    int linha;
} CodigoFonte;

typedef enum { TIPO_INT, TIPO_FLOAT, TIPO_ERRO } TipoDado;

typedef struct EntradaSimbolo {
    char lexema[TAM_LEXEMA];
    TipoDado tipo;
    int linha;
    struct EntradaSimbolo *prox;
} EntradaSimbolo;

typedef struct TabelaSimbolos {
    EntradaSimbolo *entradas;
    struct TabelaSimbolos *anterior;
} TabelaSimbolos;

typedef struct {
    TipoDado tipo;
    char operando[TAM_LEXEMA];
} ResultadoExpressao;

typedef struct {
    CodigoFonte *codigo;
    Token tokenAtual;
    TabelaSimbolos *tabelaAtual;
    FILE *saidaTac;
    int temErro;
    unsigned contadorTemporarios;
    unsigned contadorRotulos;
} Parser;

int abrirCodigoFonte(CodigoFonte *codigo, const char *nomeArquivo);
void fecharCodigoFonte(CodigoFonte *codigo);
Token proximoToken(CodigoFonte *codigo);
const char *nomeToken(TokenType tipo);
void erroLexico(int linha, int caractere);
void erroSintatico(int linha, TokenType esperado, TokenType encontrado);
void erroSemantico(int linha, const char *mensagem);

TabelaSimbolos *criarTabela(TabelaSimbolos *anterior);
TabelaSimbolos *destruirTabela(TabelaSimbolos *tabela);
int inserirSimbolo(TabelaSimbolos *tabela, const char *lexema, TipoDado tipo, int linha);
EntradaSimbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *lexema);

int analisarPrograma(Parser *parser);
void avancarEntrada(Parser *parser);
int casar(Parser *parser, TokenType esperado);
void Programa(Parser *parser);
void ListaDecl(Parser *parser);
void Declaracao(Parser *parser);
void ListaCmd(Parser *parser);
void Comando(Parser *parser);
void Atribuicao(Parser *parser);
void Print(Parser *parser);
void If(Parser *parser);
void While(Parser *parser);
void Bloco(Parser *parser);
void Condicao(Parser *parser, const char *rotuloVerdadeiro, const char *rotuloFalso);
TipoDado Tipo(Parser *parser);
ResultadoExpressao Expressao(Parser *parser);
ResultadoExpressao Termo(Parser *parser);
ResultadoExpressao Fator(Parser *parser);

char *novoTemporario(Parser *parser);
char *novoRotulo(Parser *parser);
void emitir(Parser *parser, const char *formato, ...);

#endif
