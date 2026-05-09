#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// Expression types
typedef enum {
    EXPR_INT_LIT,
    EXPR_CHAR_LIT,
    EXPR_STRING_LIT,
    EXPR_IDENT,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_ASSIGN,
    EXPR_CALL,
} ExprType;

typedef struct Expr Expr;

struct Expr {
    ExprType type;
    int line;
    int col;

    union {
        int int_val;
        char char_val;
        struct { char *str_val; int str_len; };
        char *ident;
        struct { TokenType bin_op; Expr *lhs; Expr *rhs; };
        struct { TokenType unary_op; Expr *operand; };
        struct { Expr *assign_target; Expr *assign_val; };
        struct { char *call_name; Expr **call_args; int num_args; };
    };
};

// Statement types
typedef enum {
    STMT_RETURN,
    STMT_EXPR,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BLOCK,
    STMT_VAR_DECL,
} StmtType;

typedef struct Stmt Stmt;

struct Stmt {
    StmtType type;
    int line;
    int col;

    union {
        Expr *return_expr;
        Expr *expr;
        struct { Expr *if_cond; Stmt *then_body; Stmt *else_body; };
        struct { Expr *while_cond; Stmt *while_body; };
        struct { Stmt *for_init; Expr *for_cond; Expr *for_update; Stmt *for_body; };
        struct { Stmt **block_stmts; int num_block_stmts; };
        struct { TokenType var_type; char *var_name; Expr *var_init; };
    };
};

typedef struct {
    TokenType param_type;
    char *param_name;
} Param;

typedef struct {
    TokenType return_type;
    char *name;
    Param *params;
    int num_params;
    Stmt **body;
    int num_stmts;
} Function;

typedef struct {
    Function *functions;
    int num_functions;
} ASTTree;

ASTTree *parse(Lexer *l);
void print_ast(ASTTree *tree);

#endif
