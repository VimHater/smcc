#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple variable tracking
typedef struct {
    char* name;
    int offset;  // from $sp
} Var;

typedef struct {
    Var vars[256];
    int num_vars;
    int stack_size;
    int next_offset;
    int label_count;
} CodegenCtx;

static int ctx_add_var(CodegenCtx* ctx, const char* name) {
    ctx->next_offset += 4;
    int offset = ctx->next_offset;
    ctx->vars[ctx->num_vars].name = strdup(name);
    ctx->vars[ctx->num_vars].offset = offset;
    ctx->num_vars++;
    return offset;
}

static int ctx_find_var(CodegenCtx* ctx, const char* name) {
    for (int i = 0; i < ctx->num_vars; i++) {
        if (strcmp(ctx->vars[i].name, name) == 0) return ctx->vars[i].offset;
    }
    fprintf(stderr, "codegen: undefined variable '%s'\n", name);
    exit(1);
}

static int new_label(CodegenCtx* ctx) {
    return ctx->label_count++;
}

// Push register onto stack
static void stack_push(const char *reg)
{
    printf("    addiu $sp, $sp, -4\n");
    printf("    sw %s, 0($sp)\n", reg);
}

// Pop top of stack into register
static void stack_pop(const char *reg)
{
    printf("    lw %s, 0($sp)\n", reg);
    printf("    addiu $sp, $sp, 4\n");
}

// Set up function stack frame: save $ra, $fp, set $fp
static void stack_prologue(int frame)
{
    printf("    addiu $sp, $sp, -%d\n", frame);
    printf("    sw $ra, 0($sp)\n");
    printf("    sw $fp, 4($sp)\n");
    printf("    move $fp, $sp\n");
}

// Tear down function stack frame: restore $sp, $ra, $fp, return
static void stack_epilogue(int frame)
{
    printf("    move $sp, $fp\n");
    printf("    lw $ra, 0($sp)\n");
    printf("    lw $fp, 4($sp)\n");
    printf("    addiu $sp, $sp, %d\n", frame);
    printf("    jr $ra\n");
}

// Built-in function table
typedef void (*BuiltinEmit)(CodegenCtx *ctx, Expr *e);

static void emit_builtin_exit(CodegenCtx *ctx, Expr *e);
static void emit_builtin_putchar(CodegenCtx *ctx, Expr *e);
static void emit_builtin_getchar(CodegenCtx *ctx, Expr *e);
static void emit_builtin_print_int(CodegenCtx *ctx, Expr *e);

static void emit_expr(CodegenCtx *ctx, Expr *e);

typedef struct {
    const char *name;
    int num_args; // -1 = variadic
    BuiltinEmit emit;
} Builtin;

static Builtin builtins[] = {
    {"exit",      1, emit_builtin_exit},
    {"putchar",   1, emit_builtin_putchar},
    {"getchar",   0, emit_builtin_getchar},
    {"print_int", 1, emit_builtin_print_int},
    {NULL, 0, NULL},
};

static Builtin *find_builtin(const char *name)
{
    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(builtins[i].name, name) == 0)
            return &builtins[i];
    }
    return NULL;
}

static void emit_builtin_exit(CodegenCtx *ctx, Expr *e)
{
    emit_expr(ctx, e->call_args[0]);
    printf("    move $a0, $t0\n");
    printf("    li $v0, 4001\n");
    printf("    syscall\n");
}

static void emit_builtin_putchar(CodegenCtx *ctx, Expr *e)
{
    emit_expr(ctx, e->call_args[0]);
    printf("    move $a0, $t0\n");
    printf("    li $v0, 4004\n");    // sys_write
    printf("    addiu $sp, $sp, -4\n");
    printf("    sb $t0, 0($sp)\n");  // store char on stack
    printf("    move $a1, $sp\n");   // buf = $sp
    printf("    li $a0, 1\n");       // fd = stdout
    printf("    li $a2, 1\n");       // len = 1
    printf("    syscall\n");
    printf("    addiu $sp, $sp, 4\n");
}

static void emit_builtin_getchar(CodegenCtx *ctx, Expr *e)
{
    (void)ctx; (void)e;
    printf("    li $v0, 4003\n");    // sys_read
    printf("    li $a0, 0\n");       // fd = stdin
    printf("    addiu $sp, $sp, -4\n");
    printf("    move $a1, $sp\n");   // buf = $sp
    printf("    li $a2, 1\n");       // len = 1
    printf("    syscall\n");
    printf("    lb $t0, 0($sp)\n");  // load char read
    printf("    addiu $sp, $sp, 4\n");
}

static void emit_builtin_print_int(CodegenCtx *ctx, Expr *e)
{
    // Print integer by converting digits on stack
    emit_expr(ctx, e->call_args[0]);
    int lbl = new_label(ctx);

    // handle negative
    printf("    move $t1, $t0\n");          // save original
    printf("    slt $t2, $t0, $zero\n");    // $t2 = (n < 0)
    printf("    beq $t2, $zero, .Lpi_pos_%d\n", lbl);
    printf("    sub $t0, $zero, $t0\n");    // negate
    // print '-'
    printf("    addiu $sp, $sp, -4\n");
    printf("    li $t3, 45\n");             // '-'
    printf("    sb $t3, 0($sp)\n");
    printf("    li $v0, 4004\n");
    printf("    li $a0, 1\n");
    printf("    move $a1, $sp\n");
    printf("    li $a2, 1\n");
    printf("    syscall\n");
    printf("    addiu $sp, $sp, 4\n");

    printf(".Lpi_pos_%d:\n", lbl);
    // push digits onto stack
    printf("    move $t1, $zero\n");        // digit count
    printf("    li $t4, 10\n");
    printf(".Lpi_loop_%d:\n", lbl);
    printf("    div $t0, $t4\n");
    printf("    mfhi $t3\n");               // remainder = digit
    printf("    mflo $t0\n");               // quotient
    printf("    addiu $t3, $t3, 48\n");     // + '0'
    printf("    addiu $sp, $sp, -4\n");
    printf("    sb $t3, 0($sp)\n");
    printf("    addiu $t1, $t1, 1\n");
    printf("    bne $t0, $zero, .Lpi_loop_%d\n", lbl);

    // print digits from stack
    printf(".Lpi_print_%d:\n", lbl);
    printf("    beq $t1, $zero, .Lpi_done_%d\n", lbl);
    printf("    li $v0, 4004\n");
    printf("    li $a0, 1\n");
    printf("    move $a1, $sp\n");
    printf("    li $a2, 1\n");
    printf("    syscall\n");
    printf("    addiu $sp, $sp, 4\n");
    printf("    addiu $t1, $t1, -1\n");
    printf("    j .Lpi_print_%d\n", lbl);
    printf(".Lpi_done_%d:\n", lbl);
}

// Emit expression, result in $t0
static void emit_expr(CodegenCtx* ctx, Expr* e) {
    if (!e) return;

    switch (e->type) {
        case EXPR_INT_LIT:
            printf("    li $t0, %d\n", e->int_val);
            break;

        case EXPR_CHAR_LIT:
            printf("    li $t0, %d\n", (int)e->char_val);
            break;

        case EXPR_IDENT: {
            int offset = ctx_find_var(ctx, e->ident);
            printf("    lw $t0, %d($fp)\n", offset);
            break;
        }

        case EXPR_BINARY:
            emit_expr(ctx, e->lhs);
            stack_push("$t0");
            emit_expr(ctx, e->rhs);
            stack_pop("$t1");
            // $t1 = lhs, $t0 = rhs
            switch (e->bin_op) {
                case TOK_PLUS:
                    printf("    add $t0, $t1, $t0\n");
                    break;
                case TOK_MINUS:
                    printf("    sub $t0, $t1, $t0\n");
                    break;
                case TOK_STAR:
                    printf("    mult $t1, $t0\n");
                    printf("    mflo $t0\n");
                    break;
                case TOK_SLASH:
                    printf("    div $t1, $t0\n");
                    printf("    mflo $t0\n");
                    break;
                case TOK_PERCENT:
                    printf("    div $t1, $t0\n");
                    printf("    mfhi $t0\n");
                    break;
                case TOK_AMP:
                    printf("    and $t0, $t1, $t0\n");
                    break;
                case TOK_PIPE:
                    printf("    or $t0, $t1, $t0\n");
                    break;
                case TOK_CARET:
                    // a ^ b = (a | b) & ~(a & b)
                    printf("    or $t2, $t1, $t0\n");
                    printf("    and $t3, $t1, $t0\n");
                    printf("    nor $t3, $t3, $zero\n");
                    printf("    and $t0, $t2, $t3\n");
                    break;
                case TOK_LSHIFT:
                case TOK_RSHIFT:
                    fprintf(stderr, "[TODO]: add variable shift\n");
                    exit(1);
                case TOK_EQ: {
                    int lbl = new_label(ctx);
                    printf("    beq $t1, $t0, .Leq_true_%d\n", lbl);
                    printf("    li $t0, 0\n");
                    printf("    j .Leq_end_%d\n", lbl);
                    printf(".Leq_true_%d:\n", lbl);
                    printf("    li $t0, 1\n");
                    printf(".Leq_end_%d:\n", lbl);
                    break;
                }
                case TOK_NEQ: {
                    int lbl = new_label(ctx);
                    printf("    bne $t1, $t0, .Lneq_true_%d\n", lbl);
                    printf("    li $t0, 0\n");
                    printf("    j .Lneq_end_%d\n", lbl);
                    printf(".Lneq_true_%d:\n", lbl);
                    printf("    li $t0, 1\n");
                    printf(".Lneq_end_%d:\n", lbl);
                    break;
                }
                case TOK_LT:
                    printf("    slt $t0, $t1, $t0\n");
                    break;
                case TOK_GT:
                    printf("    slt $t0, $t0, $t1\n");
                    break;
                case TOK_LEQ: {
                    // lhs <= rhs  →  !(rhs < lhs)
                    int lbl = new_label(ctx);
                    printf("    slt $t0, $t0, $t1\n");
                    printf("    beq $t0, $zero, .Lleq_true_%d\n", lbl);
                    printf("    li $t0, 0\n");
                    printf("    j .Lleq_end_%d\n", lbl);
                    printf(".Lleq_true_%d:\n", lbl);
                    printf("    li $t0, 1\n");
                    printf(".Lleq_end_%d:\n", lbl);
                    break;
                }
                case TOK_GEQ: {
                    // lhs >= rhs  →  !(lhs < rhs)
                    int lbl = new_label(ctx);
                    printf("    slt $t0, $t1, $t0\n");
                    printf("    beq $t0, $zero, .Lgeq_true_%d\n", lbl);
                    printf("    li $t0, 0\n");
                    printf("    j .Lgeq_end_%d\n", lbl);
                    printf(".Lgeq_true_%d:\n", lbl);
                    printf("    li $t0, 1\n");
                    printf(".Lgeq_end_%d:\n", lbl);
                    break;
                }
                case TOK_AND: {
                    int lbl = new_label(ctx);
                    printf("    beq $t1, $zero, .Land_false_%d\n", lbl);
                    printf("    beq $t0, $zero, .Land_false_%d\n", lbl);
                    printf("    li $t0, 1\n");
                    printf("    j .Land_end_%d\n", lbl);
                    printf(".Land_false_%d:\n", lbl);
                    printf("    li $t0, 0\n");
                    printf(".Land_end_%d:\n", lbl);
                    break;
                }
                case TOK_OR: {
                    int lbl = new_label(ctx);
                    printf("    bne $t1, $zero, .Lor_true_%d\n", lbl);
                    printf("    bne $t0, $zero, .Lor_true_%d\n", lbl);
                    printf("    li $t0, 0\n");
                    printf("    j .Lor_end_%d\n", lbl);
                    printf(".Lor_true_%d:\n", lbl);
                    printf("    li $t0, 1\n");
                    printf(".Lor_end_%d:\n", lbl);
                    break;
                }
                default:
                    fprintf(stderr, "TODO: support other binary op\n");
                    exit(1);
            }
            break;

        case EXPR_UNARY:
            emit_expr(ctx, e->operand);
            switch (e->unary_op) {
                case TOK_MINUS:
                    printf("    sub $t0, $zero, $t0\n");
                    break;
                case TOK_TILDE:
                    printf("    nor $t0, $t0, $zero\n");
                    break;
                case TOK_BANG: {
                    int lbl = new_label(ctx);
                    printf("    beq $t0, $zero, .Lnot_true_%d\n", lbl);
                    printf("    li $t0, 0\n");
                    printf("    j .Lnot_end_%d\n", lbl);
                    printf(".Lnot_true_%d:\n", lbl);
                    printf("    li $t0, 1\n");
                    printf(".Lnot_end_%d:\n", lbl);
                    break;
                }
                default:
                    fprintf(stderr, "codegen: unsupported unary op\n");
                    exit(1);
            }
            break;

        case EXPR_ASSIGN: {
            emit_expr(ctx, e->assign_val);
            int offset = ctx_find_var(ctx, e->assign_target->ident);
            printf("    sw $t0, %d($fp)\n", offset);
            break;
        }

        case EXPR_CALL: {
            Builtin *b = find_builtin(e->call_name);
            if (b) {
                b->emit(ctx, e);
            } else {
                // regular function call
                for (int i = 0; i < e->num_args && i < 4; i++) {
                    emit_expr(ctx, e->call_args[i]);
                    printf("    move $a%d, $t0\n", i);
                }
                printf("    jal %s\n", e->call_name);
                printf("    move $t0, $v0\n");
            }
            break;
        }

        case EXPR_STRING_LIT:
            fprintf(stderr, "codegen: string literals not yet supported\n");
            exit(1);
    }
}

static void emit_stmt(CodegenCtx* ctx, Stmt* s);

static void emit_block(CodegenCtx* ctx, Stmt* s) {
    if (s->type == STMT_BLOCK) {
        for (int i = 0; i < s->num_block_stmts; i++)
            emit_stmt(ctx, s->block_stmts[i]);
    } else {
        emit_stmt(ctx, s);
    }
}

static void emit_stmt(CodegenCtx* ctx, Stmt* s) {
    if (!s) return;

    switch (s->type) {
        case STMT_RETURN:
            if (s->return_expr) {
                emit_expr(ctx, s->return_expr);
                printf("    move $v0, $t0\n");
            }
            stack_epilogue(ctx->stack_size);
            break;

        case STMT_EXPR:
            emit_expr(ctx, s->expr);
            break;

        case STMT_VAR_DECL: {
            int offset = ctx_add_var(ctx, s->var_name);
            if (s->var_init) {
                emit_expr(ctx, s->var_init);
                printf("    sw $t0, %d($fp)\n", offset);
            }
            break;
        }

        case STMT_IF: {
            int lbl = new_label(ctx);
            emit_expr(ctx, s->if_cond);
            if (s->else_body) {
                printf("    beq $t0, $zero, .Lelse_%d\n", lbl);
                emit_block(ctx, s->then_body);
                printf("    j .Lend_if_%d\n", lbl);
                printf(".Lelse_%d:\n", lbl);
                emit_block(ctx, s->else_body);
            } else {
                printf("    beq $t0, $zero, .Lend_if_%d\n", lbl);
                emit_block(ctx, s->then_body);
            }
            printf(".Lend_if_%d:\n", lbl);
            break;
        }

        case STMT_WHILE: {
            int lbl = new_label(ctx);
            printf(".Lwhile_%d:\n", lbl);
            emit_expr(ctx, s->while_cond);
            printf("    beq $t0, $zero, .Lend_while_%d\n", lbl);
            emit_block(ctx, s->while_body);
            printf("    j .Lwhile_%d\n", lbl);
            printf(".Lend_while_%d:\n", lbl);
            break;
        }

        case STMT_FOR: {
            int lbl = new_label(ctx);
            if (s->for_init) emit_stmt(ctx, s->for_init);
            printf(".Lfor_%d:\n", lbl);
            if (s->for_cond) {
                emit_expr(ctx, s->for_cond);
                printf("    beq $t0, $zero, .Lend_for_%d\n", lbl);
            }
            if (s->for_body) emit_block(ctx, s->for_body);
            if (s->for_update) emit_expr(ctx, s->for_update);
            printf("    j .Lfor_%d\n", lbl);
            printf(".Lend_for_%d:\n", lbl);
            break;
        }

        case STMT_BLOCK:
            emit_block(ctx, s);
            break;
    }
}

static void emit_function(CodegenCtx* ctx, Function* fn) {
    ctx->num_vars = 0;
    ctx->stack_size = 8;  // room for $ra + $fp

    ctx->stack_size += fn->num_params * 4;
    for (int i = 0; i < fn->num_stmts; i++) {
        if (fn->body[i]->type == STMT_VAR_DECL)
            ctx->stack_size += 4;
    }

    int frame = ctx->stack_size;
    ctx->next_offset = 4; // $ra at 0, $fp at 4, vars start at 8

    printf("%s:\n", fn->name);
    stack_prologue(frame);

    // store params from $a0-$a3
    for (int i = 0; i < fn->num_params && i < 4; i++) {
        int offset = ctx_add_var(ctx, fn->params[i].param_name);
        printf("    sw $a%d, %d($fp)\n", i, offset);
    }

    for (int i = 0; i < fn->num_stmts; i++) {
        emit_stmt(ctx, fn->body[i]);
    }
}

void codegen(ASTTree* tree) {
    printf(".option pic0\n");
    printf(".text\n");
    printf(".globl __start\n");
    printf("__start:\n");
    printf("    jal main\n");
    printf("    move $a0, $v0\n");
    printf("    li $v0, 4001\n");
    printf("    syscall\n\n");

    CodegenCtx ctx = {0};

    for (int i = 0; i < tree->num_functions; i++) {
        emit_function(&ctx, &tree->functions[i]);
    }
}
