#include <include/assembler.h>
#include <include/types.h>
#include <include/log.h>
#include <stdio.h>

LOG_TAG("Assembler");

typedef struct var_inf {
    string_t* name;
    complex_data_type_t* type;
    bool is_stack;
    int stack_offset;
    void* data;
} var_inf_t;

var_inf_t* init_var_inf(string_t* name, complex_data_type_t* type, bool is_stack, int stack_offset, void* data) {
    var_inf_t* this = calloc(1, sizeof(struct var_inf));
    this->name = name;
    this->type = type;
    this->is_stack = is_stack;
    this->stack_offset = stack_offset;
    this->data = data;
    return this;
}


assembler_t* init_assembler(analyzer_t* analyzer) {
    assembler_t* this = calloc(1, sizeof(struct assembler));
    this->analyzer = analyzer;
    this->output = init_list();
    this->errors = init_list();
    return this;
}

void assembler_report_and_exit(assembler_t* assembler, int error, void** data) {
    switch (error) {
    case ASSEMBLER_ERROR_SYMBOL_REDEFINITION:
        LOG_PUSH_ARGS("%s: Error: redefinition of symbol `%s` (first defined in %s)",
                            CAST(string_t*, data[1])->buffer, CAST(string_t*, data[0])->buffer,
                            CAST(string_t*, data[2])->buffer);
        break;
    }
    exit(1);
}

void assembler_generate_function(assembler_t* assembler, ast_t* ast, list_t* lvars) {

    list_t* vars = lvars != NULL ? lvars : init_list();

    int offset = 0;

    LIST_FOREACH(ast->childs, {
        ast_t* var = CAST(ast_t*, node->data);
        list_push(vars, init_var_inf(var->name, var->data_type, true, offset, NULL));
        offset += var->data_type->size;
    })

    list_push(assembler->output, init_string("push ebp"));

    assembler_generate(assembler, ast->value, NULL);


    list_push(assembler->output, init_string("pop ebp"));

}

void assembler_generate(assembler_t* assembler, ast_t* ast, void* data) {
    if (ast == NULL)
        return;
    switch (ast->type)
    {
    case AST_TYPE_FUNCTION:
    {
        assembler_generate_function(assembler, ast, NULL);
        break;
    }
    case AST_TYPE_COMPOUND:
    {
        LIST_FOREACH(ast->childs, { assembler_generate(assembler, CAST(ast_t*, node->data), NULL); } );
        break;
    }
    case AST_TYPE_RETURN:
    {
        assembler_generate(assembler, ast->value, NULL);
        break;
    }
    case AST_TYPE_ACCESS:
    {
        list_push(assembler->output, string_format("mov %s, [%S]", CAST(char*, data), ast->name));
        break;
    }
    case AST_TYPE_OPERATION:
    {
        ast_t* lhs = CAST(ast_t*, list_index_data(ast->childs, 0));
        assembler_generate(assembler, lhs, "eax");
        ast_t* rhs = CAST(ast_t*, list_index_data(ast->childs, 1));
        assembler_generate(assembler, rhs, "ebx");

        char* opr = NULL;
        switch(ast->name->buffer[0]) {
        case '+': opr = "add"; break;
        case '-': opr = "sub"; break;
        case '*': opr = "mul"; break;
        case '/': opr = "div"; break;
        }

        list_push(assembler->output, string_format("%s eax, ebx", opr));
        list_push(assembler->output, init_string("mov edx, eax"));

        break;
    }
    }

}
