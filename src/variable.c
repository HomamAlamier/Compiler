#include <include/variable.h>
#include <stdio.h>

static int __var_unique_id_counter__ = 0;

variable_t* init_variable(string_t* name, complex_data_type_t* type,
                        bool is_stack, bool is_constant, int stack_offset, ast_t* ast) {
    variable_t* this = calloc(1, sizeof(struct variable));
    this->unique_id = __var_unique_id_counter__++;
    this->name = name;
    this->type = type;
    this->is_stack = is_stack;
    this->is_constant = is_constant;
    this->stack_offset = stack_offset;
    this->type_size = 0;
    this->ast = ast;
    this->parent = ast ? ast->parent : NULL;
    return this;
}

void dump_variable(variable_t* var) {
    printf("--> Var <--\n");
    printf("name = %s\n", var->name->buffer);
    printf("type = %s\n", var->type->name->buffer);
    printf("is_stack = %s\n", BOOL_STR(var->is_stack));
    printf("is_constant = %s\n", BOOL_STR(var->is_constant));
    printf("stack_offset = %d\n", var->stack_offset);
}

variable_t* variable_find_variable(list_t* list, string_t* name) {
    LIST_FOREACH(list, {
    variable_t* inf = node->data;
        if (string_cmp(inf->name, name)) {
            return inf;
        }
    })
    return NULL;
}

bool ast_can_access_variable(variable_t* var, ast_t* ast) {
    if (var->parent->unique_id == ast->unique_id) {
        return true;
    }

    ast_t* c = ast->parent;
    while(c) {
        if (c->name && var->parent->name)
            printf("can_access_var: Checking (%s) == (%s)\n", c->name->buffer, var->parent->name->buffer);
        if (c->unique_id == var->parent->unique_id) {
            return true;
        }
        c = c->parent;
    }
    return false;
}
