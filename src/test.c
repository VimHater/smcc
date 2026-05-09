#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

void test()
{
    const char *src =
    "int main() \n"
    "{\n"
    "// comment\n"
    "int n = 5;\n"
    "while (n = n - 1) {\n"
    "     putchar(65);\n"
    "}\n"
    "   return 1;\n"
    "}\n";

    Lexer lex;
    lexer_init(&lex, src);
    Lexer lex2;
    lexer_init(&lex2, src);
    ASTTree *ast_tree = parse(&lex2);

    Token tok;
    printf("======== LEXER TEST =========\n");
    while (tok.type != TOK_EOF && tok.type != TOK_ERROR) {
        if (!tok.start) {
            tok = lexer_next(&lex);
        }
        printf("[line: %d, col: %-2d, len: %-2d]            ",
               tok.line, tok.col, tok.len);
        print_token(tok);
        tok = lexer_next(&lex);
    }

    printf("======== PARSER TEST =========\n");
    print_ast(ast_tree);

    printf("\n======== CODEGEN TEST =========\n");
    codegen(ast_tree);

}
