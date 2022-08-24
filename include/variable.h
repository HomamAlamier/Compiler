#ifndef VARIABLE_H
#define VARIABLE_H

#include <include/ast.h>

typedef struct variable {
    int unique_id;
    string_t* name;
    complex_data_type_t* type;
    bool is_stack;
    bool is_constant;
    int stack_offset;
    size_t type_size;
    ast_t* ast;
    ast_t* parent;
} variable_t;

variable_t* init_variable(string_t* name, complex_data_type_t* type,
                        bool is_stack, bool is_constant, int stack_offset, ast_t* ast);
void dump_variable(variable_t* var);

variable_t* variable_find_variable(list_t* list, string_t* name);
bool ast_can_access_variable(variable_t* var, ast_t* ast);

#endif // VARIABLE_H
