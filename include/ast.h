#ifndef AST_H
#define AST_H

#include <include/list.h>
#include <include/strings.h>

typedef struct lexer_token lexer_token_t;

enum ast_type {
    AST_TYPE_COMPOUND,
    AST_TYPE_LIST,
    AST_TYPE_FUNCTION,
    AST_TYPE_ASM_FUNCTION,
    AST_TYPE_VARIABLE,
    AST_TYPE_CONSTANT,
    AST_TYPE_ACCESS,
    AST_TYPE_TEMPLATE,
    AST_TYPE_RETURN,
    AST_TYPE_ASSIGNMENT,
    AST_TYPE_OPERATION,
    AST_TYPE_STATEMENT,
    AST_TYPE_INTEGER,
    AST_TYPE_DOUBLE,
    AST_TYPE_STRING,
    AST_TYPE_CALL
};

typedef struct complex_data_type {
    string_t* name;
    struct ast* template;
    int size;
} complex_data_type_t;

typedef struct ast {
    unsigned int unique_id;
    enum ast_type type;
    list_t* childs;
    string_t* name;
    struct ast* value;
    struct ast* parent;
    lexer_token_t* token;
    void* data;
    complex_data_type_t* data_type;
} ast_t;

const char* ast_type_str(int type);
complex_data_type_t* init_complex_data_type(string_t* name);
ast_t* init_ast(int type);

void dump_ast(ast_t* ast);

#endif // AST_H
