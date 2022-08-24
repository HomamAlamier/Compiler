#include <include/assembler.h>
#include <include/types.h>
#include <include/log.h>
#include <include/lexer.h>
#include <include/variable.h>
#include <assert.h>
#include <stdio.h>

LOG_TAG("Assembler");
static int __constant_id__ = 0;


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
            int written = sprintf(buf, "%x", (unsigned char)ch);
            assert(written < 3);

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

void set_var_stack_offset(assembler_t* assembler, variable_t* var, size_t offset) {
    size_t pre_offset = 0;
    LIST_FOREACH(assembler->vars_list, {
        variable_t* item = node->data;
        if (item->parent)
            printf("check parent (p1 = %d, p2 = %d)\n", var->parent->parent->unique_id, item->parent->unique_id);
        if (!item->is_constant && item->unique_id != var->unique_id && var->ast->parent->parent->unique_id == item->ast->parent->unique_id) {
            printf("shifting `%s` stack_offset (before = %d, after = %lu)\n", item->name->buffer, item->stack_offset, item->stack_offset + offset);
            item->stack_offset += offset;
        }
        else if (item->unique_id == var->unique_id)
            break;
    })
    var->stack_offset = offset;
}

assembler_t* init_assembler(analyzer_t* analyzer, codegen_t* codegen) {
    assembler_t* this = calloc(1, sizeof(struct assembler));
    this->analyzer = analyzer;
    this->output = calloc(1, sizeof(struct assembler_output));
    this->output->text = init_list();
    this->output->data = init_list();
    this->errors = init_list();
    this->vars_list = init_list();
    this->codegen = codegen;
    this->total_stack_allocation = 0;
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
        list_push(assembler->vars_list, init_variable(var->name, var->data_type, true, false, offset, var));
        offset += assembler->codegen->address_size;
    })

    list_push(assembler->output->text, string_format("_%S:", ast->name));
    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_REG, CODEGEN_REG_BASE_POINTER));
    list_t* old = assembler->output->text;
    assembler->output->text = init_list();
    assembler->total_stack_allocation = 0;

    assembler_generate(assembler, ast->value, 0);

    if (assembler->total_stack_allocation)
        list_push(old, assembler->codegen->gen_func(CODEGEN_GEN_OPER_INT_TO_REG, CODEGEN_OPER_SUB, assembler->total_stack_allocation, CODEGEN_REG_STACK_POINTER));
    list_push(old, assembler->codegen->gen_func(CODEGEN_GEN_OPER_REG_TO_REG, CODEGEN_OPER_MOV, CODEGEN_REG_STACK_POINTER, CODEGEN_REG_BASE_POINTER));

    list_append_list(old, assembler->output->text);
    list_free(&assembler->output->text, false);
    assembler->output->text = old;

    if (assembler->total_stack_allocation)
        list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_INT_TO_REG, CODEGEN_OPER_ADD, assembler->total_stack_allocation, CODEGEN_REG_STACK_POINTER));
    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_POP_REG, CODEGEN_REG_BASE_POINTER));
    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_FUNC_RET_CLEAN, offset));

}

void assembler_generate_data(assembler_t* assembler) {
    LIST_FOREACH(assembler->vars_list, {
        variable_t* inf = node->data;
        if (inf->is_constant && inf->ast->value->type == AST_TYPE_STRING) {
            string_t* hex = get_string_hex_bytes(inf->ast->value->data);
            list_push(assembler->output->data, assembler->codegen->gen_func(CODEGEN_GEN_DATA, CODEGEN_SIZE_BYTE, hex, inf->ast->name));
            string_free(&hex);
        }
    })
}

void assembler_generate_asm_function(assembler_t* assembler, ast_t* ast) {
    int offset = 0;

    LIST_FOREACH(ast->childs, {
        ast_t* var = CAST(ast_t*, node->data);
        list_push(assembler->vars_list, init_variable(var->name, var->data_type, true, false, offset, var));
        offset += assembler->codegen->address_size;
    })

    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_FUNC_SYMBOL, ast->name));
    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_REG, CODEGEN_REG_BASE_POINTER));
    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_REG_TO_REG, CODEGEN_OPER_MOV, CODEGEN_REG_STACK_POINTER, CODEGEN_REG_BASE_POINTER));

    LIST_FOREACH(ast->value->childs, {
        ast_t* item = node->data;
        list_push(assembler->output->text, item->data);
    })

    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_POP_REG, CODEGEN_REG_BASE_POINTER));
    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_FUNC_RET_CLEAN, offset));
}

void assembler_generate(assembler_t* assembler, ast_t* ast, int data) {
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
        LIST_FOREACH(ast->value->childs, {
            ast_t* item = node->data;
            switch (item->type) {
            case AST_TYPE_ACCESS_ARRAY:
            case AST_TYPE_ACCESS:
                assembler_generate(assembler, item, CODEGEN_REG_A);
                break;
            case AST_TYPE_OPERATION:
                 assembler_generate(assembler, item, data);
                 list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_REG, CODEGEN_REG_A));
                 break;
            case AST_TYPE_INTEGER:
                 list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_STR, item->token->value));
                 break;
             case AST_TYPE_STRING:
                 assembler_generate(assembler, item, 0);
                 break;
            }
        })
        list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_FUNC_CALL, ast->name));
        break;
    }
    case AST_TYPE_FUNCTION:
    {
        symbol_entry_t* e = symbol_table_find_entry(assembler->analyzer->symbol_table, ast->name);
        if (e && e->is_accessed)
            assembler_generate_function(assembler, ast);
        break;
    }
    case AST_TYPE_ASM_FUNCTION:
    {
        symbol_entry_t* e = symbol_table_find_entry(assembler->analyzer->symbol_table, ast->name);
        if (e && e->is_accessed)
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
        assembler_generate(assembler, ast->value, CODEGEN_REG_A);
        break;
    }
    case AST_TYPE_INTEGER:
    {
        list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_STR_TO_REG, CODEGEN_OPER_MOV, ast->token->value, data));
        break;
    }
    case AST_TYPE_STRING:
    {
        ast_t* tree = init_ast(AST_TYPE_CONSTANT);
        tree->name = string_format("_constant_%d%d%d", ast->token->row, ast->token->col, __constant_id__);
        __constant_id__++;
        tree->value = ast;

        list_push(assembler->vars_list, init_variable(tree->name, init_complex_data_type(init_string("string"), false, 0), false, true, 0, tree));

        if (ast->parent->type == AST_TYPE_CALL) {
            list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_STR, tree->name));
        } else {
            list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_STR_TO_REG, CODEGEN_OPER_MOV, tree->name, data));
        }

        break;
    }
    case AST_TYPE_VARIABLE_ARRAY:
    case AST_TYPE_VARIABLE:
    {
        variable_t* var = init_variable(ast->name, ast->data_type, false, false, 0, ast);
        list_push(assembler->vars_list, var);
        symbol_entry_t* e = symbol_table_find_entry(assembler->analyzer->symbol_table, ast->name);
        if (e && e->is_accessed) {

            if (ast->parent && ast->parent->parent && ast->parent->parent->type == AST_TYPE_FUNCTION) {

                int type = 0;
                int type_sz = 0;
                if (string_cmp_str(ast->data_type->name, "int"))
                    type = CODEGEN_TYPE_INT;
                if (type)
                    type_sz = assembler->codegen->size_func(type);
                else
                    type_sz = assembler->codegen->address_size;
                var->type_size = type_sz;
                int sz = type_sz;
                bool is_array = ast->type == AST_TYPE_VARIABLE_ARRAY;
                if (is_array) {
                    if (ast->index != NULL) {
                        sz *= *CAST(int*, ast->index->data);
                    } else if (ast->value && ast->value->type == AST_TYPE_LIST && ast->value->childs->size > 0) {
                        sz *= ast->value->childs->size;
                    } else {
                        sz = assembler->codegen->address_size;
                    }
                }

                set_var_stack_offset(assembler, var, sz);

                // list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_INT_TO_REG, CODEGEN_OPER_SUB, sz, CODEGEN_REG_STACK_POINTER));
                // list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_REG_TO_REG, CODEGEN_OPER_MOV, CODEGEN_REG_STACK_POINTER, CODEGEN_REG_BASE_POINTER));

                if (ast->value) {
                    if (is_array && ast->value->type == AST_TYPE_LIST) {
                        int ind = 0;
                        LIST_FOREACH(ast->value->childs, {
                            ast_t* item = node->data;
                            assembler_generate(assembler, item, CODEGEN_REG_A);
                            list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_REG_TO_STACK, CODEGEN_OPER_MOV, CODEGEN_REG_A, sz + (type_sz * (ind++))));
                        })
                    } else {
                        assembler_generate(assembler, ast->value, CODEGEN_REG_A);
                        list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_REG_TO_STACK, CODEGEN_OPER_MOV, CODEGEN_REG_A, sz));
                    }
                }
                assembler->total_stack_allocation += sz;
            }

        }
        break;
    }
    case AST_TYPE_CONSTANT:
    {
        list_push(assembler->vars_list, init_variable(ast->name, ast->data_type, false, true, 0, ast));
        break;
    }
    case AST_TYPE_ACCESS:
    case AST_TYPE_ACCESS_ARRAY:
    {
        variable_t* inf = variable_find_variable(assembler->vars_list, ast->name);
        const char* debug = ast->name->buffer;
        const char* debug2 = ast->parent && ast->parent->name ? ast->parent->name->buffer : NULL;
        if (inf != NULL && ast_can_access_variable(inf, ast)) {
            int offset = inf->stack_offset;
            if (ast->type == AST_TYPE_ACCESS_ARRAY)
                offset += inf->type_size * (*CAST(int*, ast->index->data));
            if (inf->is_stack) {

                if (ast->parent->type == AST_TYPE_CALL) {
                    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_STACK, offset));
                } else {
                    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_LOAD_FROM_STACK, data, offset));
                }

            } else if (inf->is_constant && inf->ast->value->type == AST_TYPE_STRING) {
                if (ast->parent->type == AST_TYPE_CALL) {
                    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_STR, ast->name));
                } else {
                    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_STR_TO_REG, CODEGEN_OPER_MOV, data, ast->name));
                }
            } else {
                list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_STACK_TO_REG, CODEGEN_OPER_MOV, offset, data));
                if (ast->parent->type == AST_TYPE_CALL) {
                    list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_STACK_PUSH_REG, data));
                }
                /*
                switch (inf->ast->value->type) {
                    case AST_TYPE_INTEGER:
                        list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_STR_TO_REG, CODEGEN_OPER_MOV, inf->ast->value->token->value, data));
                    break;
                }*/
            }
        } else {
            assembler_report_and_exit(assembler, ASSEMBLER_ERROR_UNDEFINED_SYMBOL, ast, NULL);
        }
        break;
    }
    case AST_TYPE_OPERATION:
    {
        ast_t* lhs = CAST(ast_t*, list_index_data(ast->childs, 0));
        assembler_generate(assembler, lhs, CODEGEN_REG_A);
        ast_t* rhs = CAST(ast_t*, list_index_data(ast->childs, 1));
        assembler_generate(assembler, rhs, CODEGEN_REG_B);

        int opr = 0;
        switch(ast->name->buffer[0]) {
        case '+': opr = CODEGEN_OPER_ADD; break;
        case '-': opr = CODEGEN_OPER_SUB; break;
        case '*': opr = CODEGEN_OPER_MUL; break;
        case '/': opr = CODEGEN_OPER_DIV; break;
        }

        list_push(assembler->output->text, assembler->codegen->gen_func(CODEGEN_GEN_OPER_REG_TO_REG, opr, CODEGEN_REG_B, CODEGEN_REG_A));

        break;
    }
    }

}

bool assembler_write_output(assembler_t* assembler, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return false;
    }
    string_t* str = assembler->codegen->gen_func(CODEGEN_GEN_FILE_HEADER, assembler->codegen);
    fwrite(str->buffer, 1, str->size, file);
    string_free(&str);

    str = assembler->codegen->gen_func(CODEGEN_GEN_SECTION, CODEGEN_SEC_TEXT);
    fwrite(str->buffer, 1, str->size, file);
    string_free(&str);
    fwrite("\n", 1, 1, file);
    LIST_FOREACH(assembler->output->text, {
        string_t* str = node->data;
        if (str->buffer[0] != '_')
            fwrite("\t", 1, 1, file);
        fwrite(str->buffer, 1, str->size, file);
        fwrite("\n", 1, 1, file);
    })

    fwrite("_start:\n\tcall _main\n\tmov ebx, eax\n\tmov eax, 1\n\tint 80h\n", 1, 55, file);

    if (assembler->output->data->size > 0)
    {
        str = assembler->codegen->gen_func(CODEGEN_GEN_SECTION, CODEGEN_SEC_DATA);
        fwrite(str->buffer, 1, str->size, file);
        fwrite("\n", 1, 1, file);
        string_free(&str);
        LIST_FOREACH(assembler->output->data, {
            string_t* str = node->data;
            fwrite("\t", 1, 1, file);
            fwrite(str->buffer, 1, str->size, file);
            fwrite("\n", 1, 1, file);
        })
    }

    fclose(file);
    return true;
}

void dump_assembler(assembler_t* assembler) {
    LIST_FOREACH(assembler->vars_list, {
        dump_variable(node->data);
    })
}
