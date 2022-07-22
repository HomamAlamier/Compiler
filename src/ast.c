#include <include/ast.h>
#include <stdio.h>
#include <include/types.h>

static int __ast_unique_id_counter = 0;

typedef struct key_val {
    const char* k;
    int v;
} key_val_t;

key_val_t type_sizes[] = {
    {"int", 4},
};

int type_sizes_len = 1;

int find_type_size(string_t* name) {
    for(int i = 0; i < type_sizes_len; ++i) {
        key_val_t* pair = &type_sizes[i];
        if (string_cmp_str(name, pair->k)) {
            return pair->v;
        }
    }
    return 0;
}


const char* ast_type_str(int type){
    switch (type) {
    case AST_TYPE_COMPOUND: return "AST_TYPE_COMPOUND";
    case AST_TYPE_LIST: return "AST_TYPE_LIST";
    case AST_TYPE_FUNCTION: return "AST_TYPE_FUNCTION";
    case AST_TYPE_ASM_FUNCTION: return "AST_TYPE_ASM_FUNCTION";
    case AST_TYPE_VARIABLE: return "AST_TYPE_VARIABLE";
    case AST_TYPE_CONSTANT: return "AST_TYPE_CONSTANT";
    case AST_TYPE_ACCESS: return "AST_TYPE_ACCESS";
    case AST_TYPE_TEMPLATE: return "AST_TYPE_TEMPLATE";
    case AST_TYPE_RETURN: return "AST_TYPE_RETURN";
    case AST_TYPE_ASSIGNMENT: return "AST_TYPE_ASSIGNMENT";
    case AST_TYPE_OPERATION: return "AST_TYPE_OPERATION";
    case AST_TYPE_STATEMENT: return "AST_TYPE_STATEMENT";
    case AST_TYPE_INTEGER: return "AST_TYPE_INTEGER";
    case AST_TYPE_DOUBLE: return "AST_TYPE_DOUBLE";
    case AST_TYPE_STRING: return "AST_TYPE_STRING";
    case AST_TYPE_CALL: return "AST_TYPE_CALL";
    }
    return string_format("Invalid (%d)", type)->buffer;
}


complex_data_type_t* init_complex_data_type(string_t* name) {
    complex_data_type_t* type = calloc(1, sizeof(struct complex_data_type));
    type->name = name;
    type->template = NULL;
    type->size = find_type_size(name);
    return type;
}

ast_t* init_ast(int type) {
    ast_t* ast = calloc(1, sizeof(struct ast));
    ast->unique_id = __ast_unique_id_counter;
    ++__ast_unique_id_counter;
    ast->type = type;
    ast->childs = init_list();
    ast->data_type = NULL;
    ast->name = NULL;
    ast->value = NULL;
    ast->parent = NULL;
    return ast;
}

char* gen_line(int c) {
    int count = c * 2;
    char* ln = calloc(count + 1, sizeof(char));
    for(int i = 0; i < count; ++i)
        ln[i] = '-';
    ln[count] = '\0';
    return ln;
}

void dump_ast_lvl(ast_t* ast, int lvl) {
    if (ast == NULL)
        return;
    char* ln = gen_line(lvl);
    printf("|%s type = %s\n", ln, ast_type_str(ast->type));
    if (ast->name)
        printf("|%s name = %s\n", ln, ast->name->buffer);
    if (ast->value) {
        printf("|%s value:\n", ln);
        dump_ast_lvl(ast->value, lvl + 1);
    }
    if (ast->type == AST_TYPE_INTEGER) {
        printf("|%s data_int: %d\n", ln, *CAST(int*, ast->data));
    }
    else if (ast->type == AST_TYPE_DOUBLE) {
        printf("|%s data_int: %f\n", ln, *CAST(double*, ast->data));
    }
    if (ast->data_type) {
        printf("|%s datatype = %s\n", ln, ast->data_type->name->buffer);
        if (ast->data_type->template) {
            printf("|%s datatype->template:\n", ln);
            dump_ast_lvl(ast->data_type->template, lvl + 1);
        }
    }
    if (ast->childs->top) {
        printf("|%s childs:\n", ln);
        list_node_t* node = ast->childs->top;
        while(node) {
            dump_ast_lvl(node->data, lvl + 1);
            node = node->next;
        }
    }
    free(ln);
}


void dump_ast(ast_t* ast) {
    dump_ast_lvl(ast, 0);
}
