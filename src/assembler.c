#include <include/assembler.h>
#include <include/types.h>
#include <include/log.h>
#include <include/lexer.h>
#include <stdio.h>

LOG_TAG("Assembler");

typedef struct var_inf {
    string_t* name;
    complex_data_type_t* type;
    bool is_stack;
    bool is_constant;
    int stack_offset;
    ast_t* ast;
    ast_t* parent;
} var_inf_t;

static int __constant_id__ = 0;

var_inf_t* init_var_inf(string_t* name, complex_data_type_t* type,
                        bool is_stack, bool is_constant, int stack_offset, ast_t* ast) {
    var_inf_t* this = calloc(1, sizeof(struct var_inf));
    this->name = name;
    this->type = type;
    this->is_stack = is_stack;
    this->is_constant = is_constant;
    this->stack_offset = stack_offset;
    this->ast = ast;
    this->parent = ast ? ast->parent : NULL;
    return this;
}

void dump_var_inf(var_inf_t* var) {
    printf("--> Var <--\n");
    printf("name = %s\n", var->name->buffer);
    printf("type = %s\n", var->type->name->buffer);
    printf("is_stack = %s\n", BOOL_STR(var->is_stack));
    printf("is_constant = %s\n", BOOL_STR(var->is_constant));
    printf("stack_offset = %d\n", var->stack_offset);
}

var_inf_t* find_var(list_t* list, string_t* name) {
    LIST_FOREACH(list, {
    var_inf_t* inf = node->data;
        if (string_cmp(inf->name, name)) {
            return inf;
        }
    })
    return NULL;
}

bool can_access_var(var_inf_t* var, ast_t* ast) {
    const char* debug = var->name->buffer;
    //printf("can_access_var: Checking (%s) == (%s)\n", var->parent->name->buffer, ast->name->buffer);
    if (var->parent->unique_id == ast->unique_id) {
        return true;
    }

    ast_t* c = ast->parent;
    while(c) {
        if (c->name)
            printf("can_access_var: Checking (%s) == (%s)\n", c->name->buffer, var->parent->name->buffer);
        if (c->unique_id == var->parent->unique_id) {
            return true;
        }
        c = c->parent;
    }
    return false;
}

string_t* get_string_hex_bytes(string_t* str) {
    string_t* out = init_string("");
    size_t pos = 0;
    while(str->buffer[pos] != '\0') {
        char ch = str->buffer[pos];
        if (ch == '\\') {
            char ch2 = str->buffer[pos + 1];
            switch (ch2) {
            case 'n': string_append_str(out, out->size > 0 ? ",0xa" : "0xa"); break;
            case '\\': string_append_str(out, out->size > 0 ? ",0x5c" : "0x5c"); break;
            case 't': string_append_str(out, out->size > 0 ? ",0x9" : "0x9"); break;
            case '0': string_append_str(out, out->size > 0 ? ",0x0" : "0x0"); break;
            case 'r': string_append_str(out, out->size > 0 ? ",0xd" : "0xd"); break;
            case 'b': string_append_str(out, out->size > 0 ? ",0x8" : "0x8"); break;
            }
            pos += 2;
        } else {

            char buf[3];
            sprintf(buf, "%x", ch);

            char hex[6];
            char id = 0;

            if (out->size > 0) {
                hex[id++] = ',';
            }

            hex[id++] = '0';
            hex[id++] = 'x';
            hex[id++] = buf[0];
            hex[id++] = buf[1];
            hex[id++] = buf[2];

            string_append_str(out, hex);
        }

        pos++;
    }
    return out;
}

assembler_t* init_assembler(analyzer_t* analyzer) {
    assembler_t* this = calloc(1, sizeof(struct assembler));
    this->analyzer = analyzer;
    this->output = calloc(1, sizeof(struct assembler_output));
    this->output->text = init_list();
    this->output->data = init_list();
    this->errors = init_list();
    this->vars_list = init_list();
    return this;
}

void assembler_report_and_exit(assembler_t* assembler, int error, ast_t* ast, void* data) {

    string_t* str = string_format("%S:%d:%d: ", ast->token->filename, ast->token->row, ast->token->col);

    switch (error) {
    case ASSEMBLER_ERROR_UNDEFINED_SYMBOL:
        string_append(str, string_format("Error: undefined symbol `%S`",
                            ast->name));
        break;
    case ASSEMBLER_ERROR_UNHANDLED_TYPE:
        string_append(str, string_format("Error: unhandled type `%s`",
                            ast_type_str(ast->type)));
        break;
    }
    LOG_PUSH(str->buffer);
    string_free(&str);
    exit(1);
}

void assembler_generate_function(assembler_t* assembler, ast_t* ast) {

    int offset = 0;

    LIST_FOREACH(ast->childs, {
        ast_t* var = CAST(ast_t*, node->data);
        list_push(assembler->vars_list, init_var_inf(var->name, var->data_type, true, false, offset, var));
        offset += var->data_type->size;
    })

    list_push(assembler->output->text, string_format("_%S:", ast->name));
    list_push(assembler->output->text, init_string("push ebp"));
    list_push(assembler->output->text, init_string("mov ebp, esp"));

    assembler_generate(assembler, ast->value, NULL);


    list_push(assembler->output->text, init_string("pop ebp"));
    list_push(assembler->output->text, init_string("ret"));

}

void assembler_generate_data(assembler_t* assembler) {
    LIST_FOREACH(assembler->vars_list, {
        var_inf_t* inf = node->data;
        if (inf->is_constant && inf->ast->value->type == AST_TYPE_STRING) {
            string_t* hex = get_string_hex_bytes(inf->ast->value->data);
            list_push(assembler->output->data, string_format("%S db %S", inf->ast->name, hex));
            string_free(&hex);
        }
    })
}

void assembler_generate_asm_function(assembler_t* assembler, ast_t* ast) {
    int offset = 0;

    LIST_FOREACH(ast->childs, {
        ast_t* var = CAST(ast_t*, node->data);
        list_push(assembler->vars_list, init_var_inf(var->name, var->data_type, true, false, offset, var));
        offset += var->data_type->size;
    })

    list_push(assembler->output->text, string_format("_%S:", ast->name));
    list_push(assembler->output->text, init_string("push ebp"));
    list_push(assembler->output->text, init_string("mov ebp, esp"));

    LIST_FOREACH(ast->value->childs, {
        ast_t* item = node->data;
        list_push(assembler->output->text, item->data);
    })

    list_push(assembler->output->text, init_string("pop ebp"));
    list_push(assembler->output->text, init_string("ret"));
}

void assembler_generate(assembler_t* assembler, ast_t* ast, void* data) {
    if (ast == NULL)
        return;
    switch (ast->type)
    {
    case AST_TYPE_CALL:
    {
        symbol_entry_t* entry = symbol_table_find_entry(assembler->analyzer->symbol_table, ast->name);
        if (entry == NULL) {
            assembler_report_and_exit(assembler, ASSEMBLER_ERROR_UNDEFINED_SYMBOL, ast, NULL);
        }
        const char* debug = ast->name->buffer;
        int stack_alloc = 0;
        LIST_FOREACH(ast->value->childs, {
            ast_t* item = node->data;
            stack_alloc += 4;
            switch (item->type) {
            case AST_TYPE_ACCESS:
                assembler_generate(assembler, item, "eax");
                list_push(assembler->output->text, init_string("push eax"));
                break;
            case AST_TYPE_OPERATION:
                 assembler_generate(assembler, item, data);
                 list_push(assembler->output->text, init_string("push eax"));
                 break;
            case AST_TYPE_INTEGER:
                 list_push(assembler->output->text, string_format("push %S", item->token->value));
                 break;
             case AST_TYPE_STRING:
                 assembler_generate(assembler, item, "eax");
                 list_push(assembler->output->text, init_string("push eax"));
                 break;
            }
        })
        list_push(assembler->output->text, string_format("call _%S", ast->name));
        list_push(assembler->output->text, string_format("add esp, %d", stack_alloc));
        break;
    }
    case AST_TYPE_FUNCTION:
    {
        assembler_generate_function(assembler, ast);
        break;
    }
    case AST_TYPE_ASM_FUNCTION:
    {
        assembler_generate_asm_function(assembler, ast);
        break;
    }
    case AST_TYPE_COMPOUND:
    {
        LIST_FOREACH(ast->childs, { assembler_generate(assembler, CAST(ast_t*, node->data), data); } );
        break;
    }
    case AST_TYPE_RETURN:
    {
        assembler_generate(assembler, ast->value, "eax");
        break;
    }
    case AST_TYPE_INTEGER:
    {
        list_push(assembler->output->text, string_format("mov %s, %S", data, ast->token->value));
        break;
    }
    case AST_TYPE_STRING:
    {
        ast_t* tree = init_ast(AST_TYPE_CONSTANT);
        tree->name = string_format("_constant_%d%d%d", ast->token->row, ast->token->col, __constant_id__);
        __constant_id__++;
        tree->value = ast;

        list_push(assembler->vars_list, init_var_inf(tree->name, init_complex_data_type(init_string("string")), false, true, 0, tree));

        if (ast->parent->type == AST_TYPE_CALL) {
            list_push(assembler->output->text, string_format("mov %s, %S", data, tree->name));
        }

        break;
    }
    case AST_TYPE_VARIABLE:
    {
        list_push(assembler->vars_list, init_var_inf(ast->name, ast->data_type, false, false, 0, ast));
        break;
    }
    case AST_TYPE_CONSTANT:
    {
        list_push(assembler->vars_list, init_var_inf(ast->name, ast->data_type, false, true, 0, ast));
        break;
    }
    case AST_TYPE_ACCESS:
    {
        var_inf_t* inf = find_var(assembler->vars_list, ast->name);
        char* reg = CAST(char*, data);
        const char* debug = ast->name->buffer;
        const char* debug2 = ast->parent && ast->parent->name ? ast->parent->name->buffer : NULL;
        if (inf != NULL && can_access_var(inf, ast)) {

            if (inf->is_stack) {
                list_push(assembler->output->text, string_format("mov %s, [ebp + %d]", reg, 8 + inf->stack_offset));
            } else if (inf->is_constant && inf->ast->value->type == AST_TYPE_STRING) {
                list_push(assembler->output->text, string_format("mov %s, %S", reg, ast->name));
            } else {
                switch (inf->ast->value->type) {
                    case AST_TYPE_INTEGER:
                        list_push(assembler->output->text, string_format("mov %s, %S", reg, inf->ast->value->token->value));
                    break;
                }
            }
        } else {
            assembler_report_and_exit(assembler, ASSEMBLER_ERROR_UNDEFINED_SYMBOL, ast, NULL);
        }
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

        list_push(assembler->output->text, string_format("%s eax, ebx", opr));

        break;
    }
    }

}

bool assembler_write_output(assembler_t* assembler, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return false;
    }
    fwrite("[bits 32]\nsection .text\n\tglobal _start\n", 1, 39, file);

    LIST_FOREACH(assembler->output->text, {
        string_t* str = node->data;
        if (str->buffer[0] != '_')
            fwrite("\t", 1, 1, file);
        fwrite(str->buffer, 1, str->size, file);
        fwrite("\n", 1, 1, file);
    })

    fwrite("_start:\n\tcall _main\n\tmov ebx, eax\n\tmov eax, 1\n\tint 80h\n", 1, 55, file);

    if (assembler->output->data->size > 0)
        fwrite("section .data\n", 1, 14, file);
    LIST_FOREACH(assembler->output->data, {
        string_t* str = node->data;
        fwrite("\t", 1, 1, file);
        fwrite(str->buffer, 1, str->size, file);
        fwrite("\n", 1, 1, file);
    })

    fclose(file);
    return true;
}

void dump_assembler(assembler_t* assembler) {
    LIST_FOREACH(assembler->vars_list, {
        dump_var_inf(node->data);
    })
}
