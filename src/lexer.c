#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void lexer_init(Lexer *l, const char *src)
{
    l->src = src;
    l->cur = src;
    l->line = 1;
    l->col = 1;
}

static Token make_token(TokenType type, const char *start, int len, int line, int col)
{
    return (Token){type, start, len, line, col};
}

static Token error_token(Lexer *l, const char *msg)
{
    return (Token){TOK_ERROR, msg, (int)strlen(msg), l->line, l->col};
}

static char peek(Lexer *l)
{
    return *l->cur;
}

static char advance(Lexer *l)
{
    char c = *l->cur++;
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static int match(Lexer *l, char expected)
{
    if (*l->cur == expected) {
        advance(l);
        return 1;
    }
    return 0;
}

static void skip_whitespace(Lexer *l)
{
    for (;;) {
        char c = peek(l);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(l);
        } else if (c == '/' && l->cur[1] == '/') {
            // Line comment
            while (peek(l) && peek(l) != '\n')
                advance(l);
        } else if (c == '/' && l->cur[1] == '*') {
            // Block comment
            advance(l);
            advance(l);
            while (peek(l)) {
                if (peek(l) == '*' && l->cur[1] == '/') {
                    advance(l);
                    advance(l);
                    break;
                }
                advance(l);
            }
        } else {
            break;
        }
    }
}

static TokenType check_keyword(const char *start, int len)
{
    static const struct { const char *kw; TokenType type; } keywords[] = {
        {"int",      TOK_INT},
        {"char",     TOK_CHAR},
        // {"float",    TOK_FLOAT},
        {"void",     TOK_VOID},
        {"return",   TOK_RETURN},
        {"if",       TOK_IF},
        {"else",     TOK_ELSE},
        {"while",    TOK_WHILE},
        {"for",      TOK_FOR},
        {"do",       TOK_DO},
        {"break",    TOK_BREAK},
        {"continue", TOK_CONTINUE},
        {"struct",   TOK_STRUCT},
        {"sizeof",   TOK_SIZEOF},
    };
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if ((int)strlen(keywords[i].kw) == len &&
            memcmp(start, keywords[i].kw, len) == 0)
            return keywords[i].type;
    }
    return TOK_IDENT;
}

static Token lex_ident(Lexer *l)
{
    const char *start = l->cur;
    int line = l->line, col = l->col;

    while (isalnum(peek(l)) || peek(l) == '_') advance(l);

    int len = (int)(l->cur - start);
    TokenType type = check_keyword(start, len);
    return make_token(type, start, len, line, col);
}

static Token lex_number(Lexer *l)
{
    const char *start = l->cur;
    int line = l->line, col = l->col;

    if (peek(l) == '0' && (l->cur[1] == 'x' || l->cur[1] == 'X')) {
        advance(l);
        advance(l);
        while (isxdigit(peek(l)))
            advance(l);
    } else {
        while (isdigit(peek(l)))
            advance(l);
    }

    return make_token(TOK_INT_LIT, start, (int)(l->cur - start), line, col);
}

static Token lex_char(Lexer *l)
{
    int line = l->line, col = l->col;
    const char *start = l->cur;

    advance(l); // opening '
    if (peek(l) == '\\')
        advance(l); // escape
    advance(l); // char
    if (peek(l) != '\'')
        return error_token(l, "unterminated char literal");
    advance(l); // closing '

    return make_token(TOK_CHAR_LIT, start, (int)(l->cur - start), line, col);
}

static Token lex_string(Lexer *l)
{
    int line = l->line, col = l->col;
    const char *start = l->cur;

    advance(l); // opening "
    while (peek(l) && peek(l) != '"') {
        if (peek(l) == '\\')
            advance(l); // skip escape
        advance(l);
    }
    if (!peek(l))
        return error_token(l, "unterminated string literal");
    advance(l); // closing "

    return make_token(TOK_STRING_LIT, start, (int)(l->cur - start), line, col);
}

Token lexer_next(Lexer *l)
{
    skip_whitespace(l);

    if (!peek(l)) {
        return make_token(TOK_EOF, l->cur, 0, l->line, l->col);
    }

    int line = l->line, col = l->col;
    const char *start = l->cur;
    char c = peek(l);

    if (isalpha(c) || c == '_')       return lex_ident(l);
    if (isdigit(c))                   return lex_number(l);
    if (c == '\'')                    return lex_char(l);
    if (c == '"')                     return lex_string(l);

    advance(l);

    switch (c) {
        case '(': return make_token(TOK_LPAREN, start, 1, line, col);
        case ')': return make_token(TOK_RPAREN, start, 1, line, col);
        case '{': return make_token(TOK_LBRACE, start, 1, line, col);
        case '}': return make_token(TOK_RBRACE, start, 1, line, col);
        case '[': return make_token(TOK_LBRACKET, start, 1, line, col);
        case ']': return make_token(TOK_RBRACKET, start, 1, line, col);
        case ';': return make_token(TOK_SEMICOLON, start, 1, line, col);
        case ',': return make_token(TOK_COMMA, start, 1, line, col);
        case '~': return make_token(TOK_TILDE, start, 1, line, col);
        case '^': return make_token(TOK_CARET, start, 1, line, col);
        case '.': return make_token(TOK_DOT, start, 1, line, col);
        case '%': return make_token(TOK_PERCENT, start, 1, line, col);

        case '+':
            if (match(l, '+')) return make_token(TOK_INC, start, 2, line, col);
            if (match(l, '=')) return make_token(TOK_PLUS_ASSIGN, start, 2, line, col);
            return make_token(TOK_PLUS, start, 1, line, col);
        case '-':
            if (match(l, '-')) return make_token(TOK_DEC, start, 2, line, col);
            if (match(l, '=')) return make_token(TOK_MINUS_ASSIGN, start, 2, line, col);
            if (match(l, '>')) return make_token(TOK_ARROW, start, 2, line, col);
            return make_token(TOK_MINUS, start, 1, line, col);
        case '*':
            if (match(l, '=')) return make_token(TOK_STAR_ASSIGN, start, 2, line, col);
            return make_token(TOK_STAR, start, 1, line, col);
        case '/':
            if (match(l, '=')) return make_token(TOK_SLASH_ASSIGN, start, 2, line, col);
            return make_token(TOK_SLASH, start, 1, line, col);
        case '=':
            if (match(l, '=')) return make_token(TOK_EQ, start, 2, line, col);
            return make_token(TOK_ASSIGN, start, 1, line, col);
        case '!':
            if (match(l, '=')) return make_token(TOK_NEQ, start, 2, line, col);
            return make_token(TOK_BANG, start, 1, line, col);
        case '<':
            if (match(l, '=')) return make_token(TOK_LEQ, start, 2, line, col);
            if (match(l, '<')) return make_token(TOK_LSHIFT, start, 2, line, col);
            return make_token(TOK_LT, start, 1, line, col);
        case '>':
            if (match(l, '=')) return make_token(TOK_GEQ, start, 2, line, col);
            if (match(l, '>')) return make_token(TOK_RSHIFT, start, 2, line, col);
            return make_token(TOK_GT, start, 1, line, col);
        case '&':
            if (match(l, '&')) return make_token(TOK_AND, start, 2, line, col);
            return make_token(TOK_AMP, start, 1, line, col);
        case '|':
            if (match(l, '|')) return make_token(TOK_OR, start, 2, line, col);
            return make_token(TOK_PIPE, start, 1, line, col);
    }

    return error_token(l, "unexpected character");
}

const char *token_type_str(TokenType type)
{
    switch (type) {
        case TOK_INT_LIT:      return "INT_LIT";
        case TOK_CHAR_LIT:     return "CHAR_LIT";
        case TOK_FLOAT_LIT:    return "FLOAT_LIT";
        case TOK_STRING_LIT:   return "STRING_LIT";
        case TOK_IDENT:        return "IDENT";
        case TOK_INT:          return "int";
        case TOK_CHAR:         return "char";
        case TOK_VOID:         return "void";
        case TOK_RETURN:       return "return";
        case TOK_IF:           return "if";
        case TOK_ELSE:         return "else";
        case TOK_WHILE:        return "while";
        case TOK_FOR:          return "for";
        case TOK_DO:           return "do";
        case TOK_BREAK:        return "break";
        case TOK_CONTINUE:     return "continue";
        case TOK_STRUCT:       return "struct";
        case TOK_SIZEOF:       return "sizeof";
        case TOK_PLUS:         return "+";
        case TOK_MINUS:        return "-";
        case TOK_STAR:         return "*";
        case TOK_SLASH:        return "/";
        case TOK_PERCENT:      return "%";
        case TOK_AMP:          return "&";
        case TOK_PIPE:         return "|";
        case TOK_CARET:        return "^";
        case TOK_TILDE:        return "~";
        case TOK_BANG:         return "!";
        case TOK_LT:           return "<";
        case TOK_GT:           return ">";
        case TOK_EQ:           return "==";
        case TOK_NEQ:          return "!=";
        case TOK_LEQ:          return "<=";
        case TOK_GEQ:          return ">=";
        case TOK_AND:          return "&&";
        case TOK_OR:           return "||";
        case TOK_ASSIGN:       return "=";
        case TOK_PLUS_ASSIGN:  return "+=";
        case TOK_MINUS_ASSIGN: return "-=";
        case TOK_STAR_ASSIGN:  return "*=";
        case TOK_SLASH_ASSIGN: return "/=";
        case TOK_INC:          return "++";
        case TOK_DEC:          return "--";
        case TOK_LSHIFT:       return "<<";
        case TOK_RSHIFT:       return ">>";
        case TOK_ARROW:        return "->";
        case TOK_DOT:          return ".";
        case TOK_LPAREN:       return "(";
        case TOK_RPAREN:       return ")";
        case TOK_LBRACE:       return "{";
        case TOK_RBRACE:       return "}";
        case TOK_LBRACKET:     return "[";
        case TOK_RBRACKET:     return "]";
        case TOK_SEMICOLON:    return ";";
        case TOK_COMMA:        return ",";
        case TOK_EOF:          return "EOF";
        case TOK_ERROR:        return "ERROR";
    }
    return "UNKNOWN";
}

void print_token(Token tok)
{
    printf("%.*s\n", tok.len, tok.start);
}
