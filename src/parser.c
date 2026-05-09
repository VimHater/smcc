#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Lexer *lexer;
    Token cur;
    Token prev;
} Parser;

static void parser_init(Parser *p, Lexer *l)
{
    p->lexer = l;
    p->cur = lexer_next(l);
}

static Token consume(Parser *p)
{
    p->prev = p->cur;
    p->cur = lexer_next(p->lexer);
    return p->prev;
}

static int check(Parser *p, TokenType type)
{
    return p->cur.type == type;
}

static Token expect(Parser *p, TokenType type)
{
    if (!check(p, type)) {
        fprintf(stderr, "%d:%d: expected '%s', got '%.*s'\n",
                p->cur.line, p->cur.col, token_type_str(type),
                p->cur.len, p->cur.start);
        exit(1);
    }
    return consume(p);
}

static char *token_to_str(Token tok)
{
    char *s = malloc(tok.len + 1);
    memcpy(s, tok.start, tok.len);
    s[tok.len] = '\0';
    return s;
}

// Forward declarations
static Expr *parse_expr(Parser *p);
static Stmt *parse_stmt(Parser *p);

// --- Expressions ---

static Expr *new_expr(ExprType type, int line, int col)
{
    Expr *e = calloc(1, sizeof(Expr));
    e->type = type;
    e->line = line;
    e->col = col;
    return e;
}

static Expr *parse_primary(Parser *p)
{
    Token tok = p->cur;

    if (check(p, TOK_INT_LIT)) {
        consume(p);
        Expr *e = new_expr(EXPR_INT_LIT, tok.line, tok.col);
        // handle hex
        if (tok.len > 2 && tok.start[0] == '0' && (tok.start[1] == 'x' || tok.start[1] == 'X'))
            e->int_val = (int)strtol(tok.start, NULL, 16);
        else
            e->int_val = (int)strtol(tok.start, NULL, 10);
        return e;
    }

    if (check(p, TOK_CHAR_LIT)) {
        consume(p);
        Expr *e = new_expr(EXPR_CHAR_LIT, tok.line, tok.col);
        if (tok.start[1] == '\\') {
            switch (tok.start[2]) {
            case 'n': e->char_val = '\n'; break;
            case 't': e->char_val = '\t'; break;
            case '\\': e->char_val = '\\'; break;
            case '\'': e->char_val = '\''; break;
            case '0': e->char_val = '\0'; break;
            default: e->char_val = tok.start[2]; break;
            }
        } else {
            e->char_val = tok.start[1];
        }
        return e;
    }

    if (check(p, TOK_STRING_LIT)) {
        consume(p);
        Expr *e = new_expr(EXPR_STRING_LIT, tok.line, tok.col);
        // strip quotes
        e->str_val = malloc(tok.len - 1);
        memcpy(e->str_val, tok.start + 1, tok.len - 2);
        e->str_val[tok.len - 2] = '\0';
        e->str_len = tok.len - 2;
        return e;
    }

    if (check(p, TOK_IDENT)) {
        consume(p);
        // function call
        if (check(p, TOK_LPAREN)) {
            consume(p);
            Expr *e = new_expr(EXPR_CALL, tok.line, tok.col);
            e->call_name = token_to_str(tok);
            e->call_args = NULL;
            e->num_args = 0;
            int cap = 4;
            e->call_args = malloc(cap * sizeof(Expr *));
            if (!check(p, TOK_RPAREN)) {
                do {
                    if (e->num_args >= cap) {
                        cap *= 2;
                        e->call_args = realloc(e->call_args, cap * sizeof(Expr *));
                    }
                    e->call_args[e->num_args++] = parse_expr(p);
                } while (check(p, TOK_COMMA) && consume(p).type == TOK_COMMA);
            }
            expect(p, TOK_RPAREN);
            return e;
        }
        Expr *e = new_expr(EXPR_IDENT, tok.line, tok.col);
        e->ident = token_to_str(tok);
        return e;
    }

    if (check(p, TOK_LPAREN)) {
        consume(p);
        Expr *e = parse_expr(p);
        expect(p, TOK_RPAREN);
        return e;
    }

    fprintf(stderr, "%d:%d: unexpected token '%.*s'\n",
            tok.line, tok.col, tok.len, tok.start);
    exit(1);
}

static Expr *parse_unary(Parser *p)
{
    if (check(p, TOK_MINUS) || check(p, TOK_BANG) || check(p, TOK_TILDE) ||
        check(p, TOK_STAR) || check(p, TOK_AMP)) {
        Token op = consume(p);
        Expr *e = new_expr(EXPR_UNARY, op.line, op.col);
        e->unary_op = op.type;
        e->operand = parse_unary(p);
        return e;
    }
    return parse_primary(p);
}

static int get_precedence(TokenType type)
{
    switch (type) {
        case TOK_OR:       return 1;
        case TOK_AND:      return 2;
        case TOK_PIPE:     return 3;
        case TOK_CARET:    return 4;
        case TOK_AMP:      return 5;
        case TOK_EQ:
        case TOK_NEQ:      return 6;
        case TOK_LT:
        case TOK_GT:
        case TOK_LEQ:
        case TOK_GEQ:      return 7;
        case TOK_LSHIFT:
        case TOK_RSHIFT:   return 8;
        case TOK_PLUS:
        case TOK_MINUS:    return 9;
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:  return 10;
        default:           return -1;
    }
}

static Expr *parse_binary(Parser *p, int min_prec)
{
    Expr *left = parse_unary(p);

    for (;;) {
        int prec = get_precedence(p->cur.type);
        if (prec < min_prec) break;

        Token op = consume(p);
        Expr *right = parse_binary(p, prec + 1);
        Expr *bin = new_expr(EXPR_BINARY, op.line, op.col);
        bin->bin_op = op.type;
        bin->lhs = left;
        bin->rhs = right;
        left = bin;
    }

    return left;
}

static Expr *parse_expr(Parser *p)
{
    Expr *left = parse_binary(p, 0);

    if (check(p, TOK_ASSIGN) || check(p, TOK_PLUS_ASSIGN) ||
        check(p, TOK_MINUS_ASSIGN) || check(p, TOK_STAR_ASSIGN) ||
        check(p, TOK_SLASH_ASSIGN)) {
        consume(p);
        Expr *e = new_expr(EXPR_ASSIGN, left->line, left->col);
        e->assign_target = left;
        e->assign_val = parse_expr(p);
        return e;
    }

    return left;
}

// --- Statements ---

static Stmt *new_stmt(StmtType type, int line, int col)
{
    Stmt *s = calloc(1, sizeof(Stmt));
    s->type = type;
    s->line = line;
    s->col = col;
    return s;
}

static Stmt *parse_block(Parser *p)
{
    Token tok = expect(p, TOK_LBRACE);
    Stmt *s = new_stmt(STMT_BLOCK, tok.line, tok.col);
    int cap = 8;
    s->block_stmts = malloc(cap * sizeof(Stmt *));
    s->num_block_stmts = 0;

    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (s->num_block_stmts >= cap) {
            cap *= 2;
            s->block_stmts = realloc(s->block_stmts, cap * sizeof(Stmt *));
        }
        s->block_stmts[s->num_block_stmts++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE);
    return s;
}

static int is_type_token(TokenType type)
{
    return type == TOK_INT || type == TOK_CHAR || type == TOK_VOID;
}

static Stmt *parse_stmt(Parser *p)
{
    Token tok = p->cur;

    if (check(p, TOK_RETURN)) {
        consume(p);
        Stmt *s = new_stmt(STMT_RETURN, tok.line, tok.col);
        if (!check(p, TOK_SEMICOLON))
            s->return_expr = parse_expr(p);
        expect(p, TOK_SEMICOLON);
        return s;
    }

    if (check(p, TOK_IF)) {
        consume(p);
        expect(p, TOK_LPAREN);
        Stmt *s = new_stmt(STMT_IF, tok.line, tok.col);
        s->if_cond = parse_expr(p);
        expect(p, TOK_RPAREN);
        s->then_body = check(p, TOK_LBRACE) ? parse_block(p) : parse_stmt(p);
        if (check(p, TOK_ELSE)) {
            consume(p);
            s->else_body = check(p, TOK_LBRACE) ? parse_block(p) : parse_stmt(p);
        }
        return s;
    }

    if (check(p, TOK_WHILE)) {
        consume(p);
        expect(p, TOK_LPAREN);
        Stmt *s = new_stmt(STMT_WHILE, tok.line, tok.col);
        s->while_cond = parse_expr(p);
        expect(p, TOK_RPAREN);
        s->while_body = check(p, TOK_LBRACE) ? parse_block(p) : parse_stmt(p);
        return s;
    }

    if (check(p, TOK_FOR)) {
        consume(p);
        expect(p, TOK_LPAREN);
        Stmt *s = new_stmt(STMT_FOR, tok.line, tok.col);
        // init
        if (!check(p, TOK_SEMICOLON))
            s->for_init = parse_stmt(p); // handles var decl or expr stmt with ;
        else
            consume(p);
        // cond
        if (!check(p, TOK_SEMICOLON))
            s->for_cond = parse_expr(p);
        expect(p, TOK_SEMICOLON);
        // update
        if (!check(p, TOK_RPAREN))
            s->for_update = parse_expr(p);
        expect(p, TOK_RPAREN);
        s->for_body = check(p, TOK_LBRACE) ? parse_block(p) : parse_stmt(p);
        return s;
    }

    if (check(p, TOK_LBRACE))
        return parse_block(p);

    // Variable declaration: int x; or int x = expr;
    if (is_type_token(tok.type)) {
        consume(p);
        Stmt *s = new_stmt(STMT_VAR_DECL, tok.line, tok.col);
        s->var_type = tok.type;
        Token name = expect(p, TOK_IDENT);
        s->var_name = token_to_str(name);
        if (check(p, TOK_ASSIGN)) {
            consume(p);
            s->var_init = parse_expr(p);
        }
        expect(p, TOK_SEMICOLON);
        return s;
    }

    // Expression statement
    Stmt *s = new_stmt(STMT_EXPR, tok.line, tok.col);
    s->expr = parse_expr(p);
    expect(p, TOK_SEMICOLON);
    return s;
}

// --- Top level ---

static Function parse_function(Parser *p)
{
    Function fn = {0};
    Token ret = consume(p);
    fn.return_type = ret.type;

    Token name = expect(p, TOK_IDENT);
    fn.name = token_to_str(name);

    expect(p, TOK_LPAREN);

    // params
    int cap = 4;
    fn.params = malloc(cap * sizeof(Param));
    while (!check(p, TOK_RPAREN)) {
        if (fn.num_params > 0)
            expect(p, TOK_COMMA);
        if (fn.num_params >= cap) {
            cap *= 2;
            fn.params = realloc(fn.params, cap * sizeof(Param));
        }
        Token ptype = consume(p);
        Token pname = expect(p, TOK_IDENT);
        fn.params[fn.num_params].param_type = ptype.type;
        fn.params[fn.num_params].param_name = token_to_str(pname);
        fn.num_params++;
    }
    expect(p, TOK_RPAREN);

    // body
    expect(p, TOK_LBRACE);
    int bcap = 8;
    fn.body = malloc(bcap * sizeof(Stmt *));
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (fn.num_stmts >= bcap) {
            bcap *= 2;
            fn.body = realloc(fn.body, bcap * sizeof(Stmt *));
        }
        fn.body[fn.num_stmts++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE);

    return fn;
}

ASTTree *parse(Lexer *l)
{
    Parser p;
    parser_init(&p, l);

    ASTTree *prog = calloc(1, sizeof(ASTTree));
    int cap = 8;
    prog->functions = malloc(cap * sizeof(Function));

    while (!check(&p, TOK_EOF)) {
        if (prog->num_functions >= cap) {
            cap *= 2;
            prog->functions = realloc(prog->functions, cap * sizeof(Function));
        }
        prog->functions[prog->num_functions++] = parse_function(&p);
    }

    return prog;
}

static void indent(int depth)
{
    for (int i = 0; i < depth; i++)
        printf("  ");
}

static void print_expr(Expr *e, int depth)
{
    if (!e) return;
    indent(depth);
    switch (e->type) {
    case EXPR_INT_LIT:
        printf("IntLit: %d\n", e->int_val);
        break;
    case EXPR_CHAR_LIT:
        printf("CharLit: '%c'\n", e->char_val);
        break;
    case EXPR_STRING_LIT:
        printf("StringLit: \"%s\"\n", e->str_val);
        break;
    case EXPR_IDENT:
        printf("Ident: %s\n", e->ident);
        break;
    case EXPR_BINARY:
        printf("Binary(%s)\n", token_type_str(e->bin_op));
        print_expr(e->lhs, depth + 1);
        print_expr(e->rhs, depth + 1);
        break;
    case EXPR_UNARY:
        printf("Unary(%s)\n", token_type_str(e->unary_op));
        print_expr(e->operand, depth + 1);
        break;
    case EXPR_ASSIGN:
        printf("Assign\n");
        print_expr(e->assign_target, depth + 1);
        print_expr(e->assign_val, depth + 1);
        break;
    case EXPR_CALL:
        printf("Call: %s\n", e->call_name);
        for (int i = 0; i < e->num_args; i++)
            print_expr(e->call_args[i], depth + 1);
        break;
    }
}

static void print_stmt(Stmt *s, int depth)
{
    if (!s) return;
    indent(depth);
    switch (s->type) {
    case STMT_RETURN:
        printf("Return\n");
        print_expr(s->return_expr, depth + 1);
        break;
    case STMT_EXPR:
        printf("ExprStmt\n");
        print_expr(s->expr, depth + 1);
        break;
    case STMT_IF:
        printf("If\n");
        print_expr(s->if_cond, depth + 1);
        indent(depth); printf("Then\n");
        print_stmt(s->then_body, depth + 1);
        if (s->else_body) {
            indent(depth); printf("Else\n");
            print_stmt(s->else_body, depth + 1);
        }
        break;
    case STMT_WHILE:
        printf("While\n");
        print_expr(s->while_cond, depth + 1);
        print_stmt(s->while_body, depth + 1);
        break;
    case STMT_FOR:
        printf("For\n");
        if (s->for_init) print_stmt(s->for_init, depth + 1);
        if (s->for_cond) print_expr(s->for_cond, depth + 1);
        if (s->for_update) print_expr(s->for_update, depth + 1);
        print_stmt(s->for_body, depth + 1);
        break;
    case STMT_BLOCK:
        printf("Block\n");
        for (int i = 0; i < s->num_block_stmts; i++)
            print_stmt(s->block_stmts[i], depth + 1);
        break;
    case STMT_VAR_DECL:
        printf("VarDecl: %s %s\n", token_type_str(s->var_type), s->var_name);
        if (s->var_init)
            print_expr(s->var_init, depth + 1);
        break;
    }
}

void print_ast(ASTTree *tree)
{
    for (int i = 0; i < tree->num_functions; i++) {
        Function *fn = &tree->functions[i];
        printf("Function: %s(", fn->name);
        for (int j = 0; j < fn->num_params; j++) {
            if (j > 0) printf(", ");
            printf("%s %s", token_type_str(fn->params[j].param_type),
                   fn->params[j].param_name);
        }
        printf(") -> %s\n", token_type_str(fn->return_type));
        for (int j = 0; j < fn->num_stmts; j++)
            print_stmt(fn->body[j], 1);
    }
}
