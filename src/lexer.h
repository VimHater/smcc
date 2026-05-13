#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    // Literals
    TOK_INT_LIT,
    TOK_CHAR_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING_LIT,
    TOK_IDENT,

    // Keywords
    TOK_INT,
    TOK_CHAR,
    TOK_FLOAT,
    TOK_VOID,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_DO,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_STRUCT,
    TOK_SIZEOF,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_AMP,
    TOK_PIPE,
    TOK_CARET,
    TOK_TILDE,
    TOK_BANG,
    TOK_LT,
    TOK_GT,
    TOK_EQ,
    TOK_NEQ,
    TOK_LEQ,
    TOK_GEQ,
    TOK_AND,
    TOK_OR,
    TOK_ASSIGN,
    TOK_PLUS_ASSIGN,
    TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN,
    TOK_SLASH_ASSIGN,
    TOK_INC,
    TOK_DEC,
    TOK_LSHIFT,
    TOK_RSHIFT,
    TOK_ARROW,
    TOK_DOT,

    // Delimiters
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_SEMICOLON,
    TOK_COMMA,

    TOK_EOF,
    TOK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int len;
    int line;
    int col;
} Token;

typedef struct {
    const char *src;
    const char *cur;
    int line;
    int col;
} Lexer;

void lexer_init(Lexer *l, const char *src);
Token lexer_next(Lexer *l);
const char *token_type_str(TokenType type);
void print_token(Token tok);

#endif
