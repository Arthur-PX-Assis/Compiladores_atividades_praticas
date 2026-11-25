#include "csmall.h"
#include <stdarg.h>

static int lerCaractere(CodigoFonte *codigo) {
    int c = fgetc(codigo->arquivo);
    if (c == '\n') codigo->linha++;
    return c;
}

static int olharCaractere(CodigoFonte *codigo) {
    int c = lerCaractere(codigo);
    if (c != EOF) {
        ungetc(c, codigo->arquivo);
        if (c == '\n') codigo->linha--;
    }
    return c;
}

int abrirCodigoFonte(CodigoFonte *codigo, const char *nomeArquivo) {
    codigo->arquivo = fopen(nomeArquivo, "r");
    codigo->linha = 1;
    if (!codigo->arquivo) {
        fprintf(stderr, "Não foi possível abrir o arquivo: %s\n", nomeArquivo);
        return 0;
    }
    return 1;
}

void fecharCodigoFonte(CodigoFonte *codigo) {
    if (codigo->arquivo) fclose(codigo->arquivo);
    codigo->arquivo = NULL;
}

const char *nomeToken(TokenType tipo) {
    static const char *nomes[] = {
        "INT", "FLOAT", "IF", "ELSE", "WHILE", "PRINT", "IDENTIFIER",
        "INT_CONST", "FLOAT_CONST", "PLUS", "MINUS", "MULT", "DIV",
        "ASSIGN", "LT", "LE", "GT", "GE", "EQ", "NE", "LPAREN",
        "RPAREN", "LBRACE", "RBRACE", "SEMICOLON", "EOF", "INVALID"
    };
    if (tipo < TOK_INT || tipo > TOK_INVALID) return "UNKNOWN";
    return nomes[tipo];
}

void erroLexico(int linha, int caractere) {
    if (isprint((unsigned char)caractere))
        fprintf(stderr, "Erro léxico na linha %d: caractere inválido '%c'.\n", linha, caractere);
    else
        fprintf(stderr, "Erro léxico na linha %d: caractere inválido.\n", linha);
}

void erroSintatico(int linha, TokenType esperado, TokenType encontrado) {
    fprintf(stderr, "Erro sintático na linha %d: esperado %s, encontrado %s.\n",
            linha, nomeToken(esperado), nomeToken(encontrado));
}

void erroSemantico(int linha, const char *mensagem) {
    fprintf(stderr, "Erro semântico na linha %d: %s\n", linha, mensagem);
}

static Token criarToken(TokenType tipo, const char *lexema, int linha) {
    Token t;
    t.tipo = tipo;
    t.linha = linha;
    snprintf(t.lexema, sizeof(t.lexema), "%s", lexema);
    return t;
}

Token proximoToken(CodigoFonte *codigo) {
    int c;
    while ((c = lerCaractere(codigo)) != EOF) {
        if (isspace((unsigned char)c)) continue;
        int linha = codigo->linha;

        if (c == '/' && olharCaractere(codigo) == '/') {
            (void)lerCaractere(codigo);
            while ((c = lerCaractere(codigo)) != EOF && c != '\n') {}
            continue;
        }

        if (isalpha((unsigned char)c) || c == '_') {
            char lex[TAM_LEXEMA];
            size_t n = 0;
            do {
                if (n < sizeof(lex) - 1) lex[n++] = (char)c;
                c = olharCaractere(codigo);
                if (isalnum((unsigned char)c) || c == '_') c = lerCaractere(codigo);
                else break;
            } while (1);
            lex[n] = '\0';
            TokenType tipo = TOK_IDENTIFIER;
            if (!strcmp(lex, "int")) tipo = TOK_INT;
            else if (!strcmp(lex, "float")) tipo = TOK_FLOAT;
            else if (!strcmp(lex, "if")) tipo = TOK_IF;
            else if (!strcmp(lex, "else")) tipo = TOK_ELSE;
            else if (!strcmp(lex, "while")) tipo = TOK_WHILE;
            else if (!strcmp(lex, "print")) tipo = TOK_PRINT;
            return criarToken(tipo, lex, linha);
        }

        if (isdigit((unsigned char)c)) {
            char lex[TAM_LEXEMA];
            size_t n = 0;
            int ehFloat = 0;
            do {
                if (n < sizeof(lex) - 1) lex[n++] = (char)c;
                c = olharCaractere(codigo);
                if (isdigit((unsigned char)c)) c = lerCaractere(codigo);
                else if (c == '.' && !ehFloat) {
                    ehFloat = 1;
                    c = lerCaractere(codigo);
                    if (!isdigit((unsigned char)olharCaractere(codigo))) {
                        erroLexico(linha, '.');
                        return criarToken(TOK_INVALID, ".", linha);
                    }
                } else break;
            } while (1);
            lex[n] = '\0';
            return criarToken(ehFloat ? TOK_FLOAT_CONST : TOK_INT_CONST, lex, linha);
        }

        char lex[3] = {(char)c, '\0', '\0'};
        switch (c) {
            case '+': return criarToken(TOK_PLUS, lex, linha);
            case '-': return criarToken(TOK_MINUS, lex, linha);
            case '*': return criarToken(TOK_MULT, lex, linha);
            case '/': return criarToken(TOK_DIV, lex, linha);
            case '(': return criarToken(TOK_LPAREN, lex, linha);
            case ')': return criarToken(TOK_RPAREN, lex, linha);
            case '{': return criarToken(TOK_LBRACE, lex, linha);
            case '}': return criarToken(TOK_RBRACE, lex, linha);
            case ';': return criarToken(TOK_SEMICOLON, lex, linha);
            case '=': case '<': case '>': case '!': {
                int p = olharCaractere(codigo);
                if (p == '=') {
                    (void)lerCaractere(codigo);
                    lex[1] = '=';
                    lex[2] = '\0';
                }
                if (c == '=' && lex[1] == '=') return criarToken(TOK_EQ, lex, linha);
                if (c == '=' && lex[1] == '\0') return criarToken(TOK_ASSIGN, lex, linha);
                if (c == '<' && lex[1] == '=') return criarToken(TOK_LE, lex, linha);
                if (c == '<') return criarToken(TOK_LT, lex, linha);
                if (c == '>' && lex[1] == '=') return criarToken(TOK_GE, lex, linha);
                if (c == '>') return criarToken(TOK_GT, lex, linha);
                if (c == '!' && lex[1] == '=') return criarToken(TOK_NE, lex, linha);
                break;
            }
        }
        erroLexico(linha, c);
        return criarToken(TOK_INVALID, lex, linha);
    }
    return criarToken(TOK_EOF, "EOF", codigo->linha);
}

void avancarEntrada(Parser *parser) { parser->tokenAtual = proximoToken(parser->codigo); }

int casar(Parser *parser, TokenType esperado) {
    if (parser->tokenAtual.tipo == esperado) {
        avancarEntrada(parser);
        return 1;
    }
    erroSintatico(parser->tokenAtual.linha, esperado, parser->tokenAtual.tipo);
    parser->temErro = 1;
    return 0;
}

TabelaSimbolos *criarTabela(TabelaSimbolos *anterior) {
    TabelaSimbolos *t = malloc(sizeof(*t));
    if (!t) { perror("malloc"); exit(EXIT_FAILURE); }
    t->entradas = NULL;
    t->anterior = anterior;
    return t;
}

TabelaSimbolos *destruirTabela(TabelaSimbolos *tabela) {
    if (!tabela) return NULL;
    TabelaSimbolos *anterior = tabela->anterior;
    EntradaSimbolo *e = tabela->entradas;
    while (e) { EntradaSimbolo *prox = e->prox; free(e); e = prox; }
    free(tabela);
    return anterior;
}

int inserirSimbolo(TabelaSimbolos *tabela, const char *lexema, TipoDado tipo, int linha) {
    for (EntradaSimbolo *e = tabela->entradas; e; e = e->prox)
        if (!strcmp(e->lexema, lexema)) return 0;
    EntradaSimbolo *novo = malloc(sizeof(*novo));
    if (!novo) { perror("malloc"); exit(EXIT_FAILURE); }
    snprintf(novo->lexema, sizeof(novo->lexema), "%s", lexema);
    novo->tipo = tipo; novo->linha = linha;
    novo->prox = tabela->entradas; tabela->entradas = novo;
    return 1;
}

EntradaSimbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *lexema) {
    for (TabelaSimbolos *escopo = tabela; escopo; escopo = escopo->anterior)
        for (EntradaSimbolo *e = escopo->entradas; e; e = e->prox)
            if (!strcmp(e->lexema, lexema)) return e;
    return NULL;
}

char *novoTemporario(Parser *parser) {
    char *nome = malloc(TAM_LEXEMA);
    if (!nome) { perror("malloc"); exit(EXIT_FAILURE); }
    snprintf(nome, TAM_LEXEMA, "t%u", ++parser->contadorTemporarios);
    return nome;
}

char *novoRotulo(Parser *parser) {
    char *nome = malloc(TAM_LEXEMA);
    if (!nome) { perror("malloc"); exit(EXIT_FAILURE); }
    snprintf(nome, TAM_LEXEMA, "L%u", ++parser->contadorRotulos);
    return nome;
}

void emitir(Parser *parser, const char *formato, ...) {
    if (!parser->saidaTac) return;
    va_list args;
    va_start(args, formato);
    vfprintf(parser->saidaTac, formato, args);
    va_end(args);
    fputc('\n', parser->saidaTac);
}

static ResultadoExpressao resultado(TipoDado tipo, const char *operando) {
    ResultadoExpressao r;
    r.tipo = tipo;
    snprintf(r.operando, sizeof(r.operando), "%s", operando ? operando : "?");
    return r;
}

static TipoDado combinarTipos(TipoDado a, TipoDado b) {
    if (a == TIPO_ERRO || b == TIPO_ERRO) return TIPO_ERRO;
    return (a == TIPO_FLOAT || b == TIPO_FLOAT) ? TIPO_FLOAT : TIPO_INT;
}

void Programa(Parser *parser) { ListaDecl(parser); ListaCmd(parser); }

void ListaDecl(Parser *parser) {
    while (!parser->temErro && (parser->tokenAtual.tipo == TOK_INT || parser->tokenAtual.tipo == TOK_FLOAT))
        Declaracao(parser);
}

TipoDado Tipo(Parser *parser) {
    if (parser->tokenAtual.tipo == TOK_INT) { casar(parser, TOK_INT); return TIPO_INT; }
    if (parser->tokenAtual.tipo == TOK_FLOAT) { casar(parser, TOK_FLOAT); return TIPO_FLOAT; }
    erroSintatico(parser->tokenAtual.linha, TOK_INT, parser->tokenAtual.tipo);
    parser->temErro = 1;
    return TIPO_ERRO;
}

void Declaracao(Parser *parser) {
    int linha = parser->tokenAtual.linha;
    TipoDado tipo = Tipo(parser);
    if (parser->temErro) return;
    if (parser->tokenAtual.tipo != TOK_IDENTIFIER) { casar(parser, TOK_IDENTIFIER); return; }
    char id[TAM_LEXEMA]; snprintf(id, sizeof(id), "%s", parser->tokenAtual.lexema);
    casar(parser, TOK_IDENTIFIER);
    if (parser->temErro) return;
    if (!inserirSimbolo(parser->tabelaAtual, id, tipo, linha)) {
        char msg[256]; snprintf(msg, sizeof(msg), "Variável '%s' já declarada neste escopo.", id);
        erroSemantico(linha, msg); parser->temErro = 1; return;
    }
    casar(parser, TOK_SEMICOLON);
}

void ListaCmd(Parser *parser) {
    while (!parser->temErro && parser->tokenAtual.tipo != TOK_EOF && parser->tokenAtual.tipo != TOK_RBRACE) {
        if (parser->tokenAtual.tipo == TOK_IDENTIFIER || parser->tokenAtual.tipo == TOK_PRINT ||
            parser->tokenAtual.tipo == TOK_IF || parser->tokenAtual.tipo == TOK_WHILE ||
            parser->tokenAtual.tipo == TOK_LBRACE) Comando(parser);
        else { erroSintatico(parser->tokenAtual.linha, TOK_IDENTIFIER, parser->tokenAtual.tipo); parser->temErro = 1; }
    }
}

void Comando(Parser *parser) {
    switch (parser->tokenAtual.tipo) {
        case TOK_IDENTIFIER: Atribuicao(parser); break;
        case TOK_PRINT: Print(parser); break;
        case TOK_IF: If(parser); break;
        case TOK_WHILE: While(parser); break;
        case TOK_LBRACE: Bloco(parser); break;
        default: erroSintatico(parser->tokenAtual.linha, TOK_IDENTIFIER, parser->tokenAtual.tipo); parser->temErro = 1;
    }
}

void Atribuicao(Parser *parser) {
    int linha = parser->tokenAtual.linha;
    char id[TAM_LEXEMA]; snprintf(id, sizeof(id), "%s", parser->tokenAtual.lexema);
    casar(parser, TOK_IDENTIFIER);
    if (parser->temErro) return;
    EntradaSimbolo *simbolo = buscarSimbolo(parser->tabelaAtual, id);
    if (!simbolo) {
        char msg[256]; snprintf(msg, sizeof(msg), "Variável '%s' não declarada.", id);
        erroSemantico(linha, msg); parser->temErro = 1; return;
    }
    casar(parser, TOK_ASSIGN);
    if (parser->temErro) return;
    ResultadoExpressao expr = Expressao(parser);
    if (parser->temErro) return;
    casar(parser, TOK_SEMICOLON);
    if (parser->temErro) return;
    if (simbolo->tipo != expr.tipo) {
        char msg[256]; snprintf(msg, sizeof(msg), "Incompatibilidade de tipos na atribuição para '%s'.", id);
        erroSemantico(linha, msg); parser->temErro = 1; return;
    }
    emitir(parser, "%s = %s", id, expr.operando);
}

void Print(Parser *parser) {
    casar(parser, TOK_PRINT); if (parser->temErro) return;
    casar(parser, TOK_LPAREN); if (parser->temErro) return;
    ResultadoExpressao expr = Expressao(parser); if (parser->temErro) return;
    casar(parser, TOK_RPAREN); if (parser->temErro) return;
    casar(parser, TOK_SEMICOLON); if (parser->temErro) return;
    emitir(parser, "print %s", expr.operando);
}

void Condicao(Parser *parser, const char *rotuloVerdadeiro, const char *rotuloFalso) {
    ResultadoExpressao esq = Expressao(parser); if (parser->temErro) return;
    TokenType rel = parser->tokenAtual.tipo;
    const char *op = NULL;
    switch (rel) {
        case TOK_LT: op = "<"; break; case TOK_LE: op = "<="; break;
        case TOK_GT: op = ">"; break; case TOK_GE: op = ">="; break;
        case TOK_EQ: op = "=="; break; case TOK_NE: op = "!="; break;
        default: erroSintatico(parser->tokenAtual.linha, TOK_LT, rel); parser->temErro = 1; return;
    }
    avancarEntrada(parser);
    ResultadoExpressao dir = Expressao(parser); if (parser->temErro) return;
    if (esq.tipo == TIPO_ERRO || dir.tipo == TIPO_ERRO) { parser->temErro = 1; return; }
    emitir(parser, "if %s %s %s goto %s", esq.operando, op, dir.operando, rotuloVerdadeiro);
    emitir(parser, "goto %s", rotuloFalso);
}

void If(Parser *parser) {
    casar(parser, TOK_IF); if (parser->temErro) return;
    casar(parser, TOK_LPAREN); if (parser->temErro) return;
    char *verdadeiro = novoRotulo(parser), *falso = novoRotulo(parser), *fim = novoRotulo(parser);
    Condicao(parser, verdadeiro, falso); if (parser->temErro) goto fim_memoria;
    casar(parser, TOK_RPAREN); if (parser->temErro) goto fim_memoria;
    emitir(parser, "%s:", verdadeiro);
    Bloco(parser); if (parser->temErro) goto fim_memoria;
    if (parser->tokenAtual.tipo == TOK_ELSE) {
        emitir(parser, "goto %s", fim);
        emitir(parser, "%s:", falso);
        casar(parser, TOK_ELSE); if (parser->temErro) goto fim_memoria;
        Bloco(parser); if (parser->temErro) goto fim_memoria;
        emitir(parser, "%s:", fim);
    } else {
        emitir(parser, "%s:", falso);
    }
 fim_memoria:
    free(verdadeiro); free(falso); free(fim);
}

void While(Parser *parser) {
    casar(parser, TOK_WHILE); if (parser->temErro) return;
    casar(parser, TOK_LPAREN); if (parser->temErro) return;
    char *teste = novoRotulo(parser), *corpo = novoRotulo(parser), *saida = novoRotulo(parser);
    emitir(parser, "%s:", teste);
    Condicao(parser, corpo, saida);
    if (!parser->temErro) casar(parser, TOK_RPAREN);
    if (!parser->temErro) {
        emitir(parser, "%s:", corpo);
        Bloco(parser);
        if (!parser->temErro) { emitir(parser, "goto %s", teste); emitir(parser, "%s:", saida); }
    }
    free(teste); free(corpo); free(saida);
}

void Bloco(Parser *parser) {
    casar(parser, TOK_LBRACE); if (parser->temErro) return;
    parser->tabelaAtual = criarTabela(parser->tabelaAtual);
    ListaDecl(parser);
    if (!parser->temErro) ListaCmd(parser);
    if (!parser->temErro) casar(parser, TOK_RBRACE);
    parser->tabelaAtual = destruirTabela(parser->tabelaAtual);
}

ResultadoExpressao Expressao(Parser *parser) {
    ResultadoExpressao esq = Termo(parser);
    while (!parser->temErro && (parser->tokenAtual.tipo == TOK_PLUS || parser->tokenAtual.tipo == TOK_MINUS)) {
        TokenType op = parser->tokenAtual.tipo;
        avancarEntrada(parser);
        ResultadoExpressao dir = Termo(parser);
        if (parser->temErro) return resultado(TIPO_ERRO, "?");
        char *temp = novoTemporario(parser);
        emitir(parser, "%s = %s %s %s", temp, esq.operando, op == TOK_PLUS ? "+" : "-", dir.operando);
        esq = resultado(combinarTipos(esq.tipo, dir.tipo), temp);
        free(temp);
    }
    return esq;
}

ResultadoExpressao Termo(Parser *parser) {
    ResultadoExpressao esq = Fator(parser);
    while (!parser->temErro && (parser->tokenAtual.tipo == TOK_MULT || parser->tokenAtual.tipo == TOK_DIV)) {
        TokenType op = parser->tokenAtual.tipo;
        avancarEntrada(parser);
        ResultadoExpressao dir = Fator(parser);
        if (parser->temErro) return resultado(TIPO_ERRO, "?");
        char *temp = novoTemporario(parser);
        emitir(parser, "%s = %s %s %s", temp, esq.operando, op == TOK_MULT ? "*" : "/", dir.operando);
        esq = resultado(combinarTipos(esq.tipo, dir.tipo), temp);
        free(temp);
    }
    return esq;
}

ResultadoExpressao Fator(Parser *parser) {
    int linha = parser->tokenAtual.linha;
    if (parser->tokenAtual.tipo == TOK_IDENTIFIER) {
        char id[TAM_LEXEMA]; snprintf(id, sizeof(id), "%s", parser->tokenAtual.lexema);
        casar(parser, TOK_IDENTIFIER); if (parser->temErro) return resultado(TIPO_ERRO, "?");
        EntradaSimbolo *s = buscarSimbolo(parser->tabelaAtual, id);
        if (!s) { char msg[256]; snprintf(msg, sizeof(msg), "Variável '%s' não declarada.", id); erroSemantico(linha, msg); parser->temErro = 1; return resultado(TIPO_ERRO, "?"); }
        return resultado(s->tipo, id);
    }
    if (parser->tokenAtual.tipo == TOK_INT_CONST) {
        char valor[TAM_LEXEMA]; snprintf(valor, sizeof(valor), "%s", parser->tokenAtual.lexema);
        casar(parser, TOK_INT_CONST); return resultado(TIPO_INT, valor);
    }
    if (parser->tokenAtual.tipo == TOK_FLOAT_CONST) {
        char valor[TAM_LEXEMA]; snprintf(valor, sizeof(valor), "%s", parser->tokenAtual.lexema);
        casar(parser, TOK_FLOAT_CONST); return resultado(TIPO_FLOAT, valor);
    }
    if (parser->tokenAtual.tipo == TOK_LPAREN) {
        casar(parser, TOK_LPAREN); if (parser->temErro) return resultado(TIPO_ERRO, "?");
        ResultadoExpressao r = Expressao(parser);
        if (!parser->temErro) casar(parser, TOK_RPAREN);
        return r;
    }
    erroSintatico(linha, TOK_IDENTIFIER, parser->tokenAtual.tipo);
    parser->temErro = 1;
    return resultado(TIPO_ERRO, "?");
}

int analisarPrograma(Parser *parser) {
    parser->temErro = 0;
    parser->contadorTemporarios = 0;
    parser->contadorRotulos = 0;
    parser->tabelaAtual = criarTabela(NULL);
    avancarEntrada(parser);
    Programa(parser);
    if (!parser->temErro && parser->tokenAtual.tipo != TOK_EOF) {
        erroSintatico(parser->tokenAtual.linha, TOK_EOF, parser->tokenAtual.tipo);
        parser->temErro = 1;
    }
    parser->tabelaAtual = destruirTabela(parser->tabelaAtual);
    return !parser->temErro;
}
