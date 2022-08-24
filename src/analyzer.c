#include <include/analyzer.h>
#include <stdlib.h>
#include <stdio.h>
#include <include/strings.h>
#include <include/types.h>
#include <include/log.h>
#include <include/lexer.h>
#include <include/variable.h>
LOG_TAG("Analyzer");


typedef struct types_strings {
    string_t* type_string;
    string_t* type_integer;
    string_t* type_double;
} types_strings_t;
static types_strings_t* __types_strings__ = NULL;

void init_types_strings() {
    __types_strings__ = calloc(1, sizeof(struct types_strings));
    __types_strings__->type_string = init_string("string");
    __types_strings__->type_integer = init_string("int");
    __types_strings__->type_double = init_string("double");
}

bool check_types(analyzer_t* analyzer, string_t* type, ast_t* value) {

    int valType = 0;
    string_t* retType = NULL;

    if (value->type == AST_TYPE_ACCESS || value->type == AST_TYPE_ACCESS_ARRAY) {
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, value->name);
        if (e) {
            valType = e->ast->value->type;
            retType = e->type;
        } else {
            return false;
        }
    } /*else if (value->type == AST_TYPE_CALL) {
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, value->name);
        if (e) {
            valType = e->ast->type;
            retType = e->type;
        } else {
            return false;
        }
    } */else if (value->type == AST_TYPE_OPERATION) {
        LIST_FOREACH(value->childs, {
            if (!check_types(analyzer, type, node->data)) {
                return false;
            }
        })
        return true;
    } else {
        valType = value->type;
    }

    if (string_cmp_str(type, "int")) {
        return valType == AST_TYPE_INTEGER || string_cmp(type, retType);
    } else if (string_cmp_str(type, "string")) {
        return valType == AST_TYPE_STRING || string_cmp(type, retType);
    }


    return false;
}

string_t* deduce_type(analyzer_t* analyzer, ast_t* ast) {
    switch (ast->type) {
    case AST_TYPE_INTEGER:
        return __types_strings__->type_integer;
    case AST_TYPE_DOUBLE:
        return __types_strings__->type_double;
    case AST_TYPE_STRING:
        return __types_strings__->type_string;
    case AST_TYPE_CALL:
    {
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, ast->name);
        return e->ast->data_type->name;
    }
    case AST_TYPE_ACCESS:
    {
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, ast->name);
        return e->ast->data_type->name;
    }
    default:
        return string_empty();
    }
}

string_t* gen_func_sig(ast_t* root) {
    if (root->type == AST_TYPE_FUNCTION || root->type == AST_TYPE_ASM_FUNCTION) {
        string_t* args = init_string("");
        bool first = true;
        LIST_FOREACH(root->childs, {
            ast_t* var = CAST(ast_t*, node->data);
            if (first) {
                string_append(args, string_format("%S %S", var->data_type->name, var->name));
                first = false;
            } else {
                string_append(args, string_format(",%S %S", var->data_type->name, var->name));
            }
        });
        return string_format("%S %S(%S)", root->data_type->name, root->name, args);
    }
    return string_empty();
}

bool symble_entry_cmp(symbol_entry_t* e1, symbol_entry_t* e2) {
    if (string_cmp(e1->name, e2->name)) {
        return e1->ast->parent->unique_id == e2->ast->parent->unique_id;
    } else {
        return string_cmp(e1->sig, e2->sig)
                && e1->ast->parent->unique_id == e2->ast->parent->unique_id;
    }
}


analyzer_t* init_analyzer() {
    if (__types_strings__ == NULL) {
        init_types_strings();
    }

    analyzer_t* this = calloc(1, sizeof(struct analyzer));
    this->symbol_table = init_symbol_table();
    this->errors = init_list();
    this->warnings = init_list();
    return this;
}

void analyzer_report_and_exit(analyzer_t* analyzer) {
    if (analyzer->errors->size) {
        LIST_FOREACH(analyzer->errors, {
            LOG_PUSH(CAST(string_t*, node->data)->buffer);
        })
        exit(1);
    }
    if (analyzer->warnings->size) {
        LIST_FOREACH(analyzer->warnings, {
            LOG_PUSH(CAST(string_t*, node->data)->buffer);
        })
        list_free_data(analyzer->warnings);
    }
}

void analyzer_analyze_internal(analyzer_t* analyzer, ast_t* ast) {
    if (ast->type == AST_TYPE_FUNCTION || ast->type == AST_TYPE_ASM_FUNCTION) {

        list_push(analyzer->symbol_table->symbols,
                  init_symbol_entry(ast->data_type->name, ast->name, gen_func_sig(ast),
                                    ast->token->filename,
                                    ast->token->col,
                                    ast->token->row,
                                    ast, true, string_cmp_str(ast->name, "main")));

        LIST_FOREACH(ast->value->childs, {
            analyzer_analyze_internal(analyzer, node->data);
        })

    } else if (ast->type == AST_TYPE_VARIABLE || ast->type == AST_TYPE_VARIABLE_ARRAY || ast->type == AST_TYPE_CONSTANT) {

        list_push(analyzer->symbol_table->symbols,
                  init_symbol_entry(ast->data_type->name, ast->name, string_format("%S %S", ast->data_type->name, ast->name),
                                    ast->token->filename,
                                    ast->token->col,
                                    ast->token->row,
                                    ast, false, false));

    }
    LIST_FOREACH(ast->childs, {
        ast_t* tree = CAST(ast_t*, node->data);
        analyzer_analyze_internal(analyzer, tree);
    });
}

void analyzer_analyze(analyzer_t* analyzer, ast_t* ast) {
    analyzer_analyze_internal(analyzer, ast);
    analyzer_report_and_exit(analyzer);


    printf("-->Symbols<--\n");
    for(size_t i = 0; i < analyzer->symbol_table->symbols->size; ++i) {
        symbol_entry_t* entry = CAST(symbol_entry_t*, list_index_data(analyzer->symbol_table->symbols, i));
        printf("%s\n", entry->name->buffer);
    }


    analyzer_check_redefinetion(analyzer);
    analyzer_report_and_exit(analyzer);

    analyzer_check_calls(analyzer, ast);
    analyzer_report_and_exit(analyzer);

    analyzer_check_var_access(analyzer, ast);
    analyzer_report_and_exit(analyzer);

    analyzer_check_types(analyzer, ast);
    analyzer_report_and_exit(analyzer);

    analyzer_generate_warnings(analyzer);
    analyzer_report_and_exit(analyzer);
}

void analyzer_check_redefinetion(analyzer_t* analyzer) {
    for(size_t i = 0; i < analyzer->symbol_table->symbols->size; ++i) {
        symbol_entry_t* entry = CAST(symbol_entry_t*, list_index_data(analyzer->symbol_table->symbols, i));
        for(size_t j = i + 1; j < analyzer->symbol_table->symbols->size; ++j) {
            symbol_entry_t* entry2 = CAST(symbol_entry_t*, list_index_data(analyzer->symbol_table->symbols, j));

            if (symble_entry_cmp(entry, entry2)) {
                list_push(analyzer->errors,
                          string_format("%S:%d:%d: Error: redefinition of symbol `%S` (first defined in %S:%d:%d)",
                          entry2->file, entry2->row, entry2->col, entry->name,
                          entry->file, entry->row, entry->col));
            }
        }
    }
}

void analyzer_check_calls(analyzer_t* analyzer, ast_t* ast) {
    if (ast->type == AST_TYPE_CALL) {
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, ast->name);
        if (e) {
            e->is_accessed = true;
            if (e->ast->childs->size != ast->value->childs->size) {
                list_push(analyzer->errors,
                          string_format("%S:%d:%d: Error: calling function `%S` with fewer arguments (has %d expected %d)",
                            ast->token->filename, ast->token->row, ast->token->col, ast->name,
                                       e->ast->childs->size, ast->value->childs->size));
            }
        } else {
            list_push(analyzer->errors,
                      string_format("%S:%d:%d: Error: undefined symbol `%S`",
                        ast->token->filename, ast->token->row, ast->token->col, ast->name));
        }
    }
    LIST_FOREACH(ast->childs, {
        ast_t* item = node->data;
        analyzer_check_calls(analyzer, item);
    })
    if (ast->type == AST_TYPE_FUNCTION) {
        LIST_FOREACH(ast->value->childs, {
            ast_t* item = node->data;
            analyzer_check_calls(analyzer, item);
        })
    }
}

void analyzer_check_var_access(analyzer_t* analyzer, ast_t* ast) {

    if (ast->type == AST_TYPE_ACCESS || ast->type == AST_TYPE_ACCESS_ARRAY
            || (
                (ast->type == AST_TYPE_VARIABLE || ast->type == AST_TYPE_VARIABLE_ARRAY)
                && (ast->parent->type == AST_TYPE_FUNCTION || ast->parent->type == AST_TYPE_ASM_FUNCTION)
                )) {
        // TODO: Check if ast can access variable
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, ast->name);
        const char* debug = ast->name->buffer;
        if (e) {
            e->is_accessed = true;
            if (ast->type == AST_TYPE_ACCESS_ARRAY) {
                int ind = *CAST(int*, ast->index->data);
                int size = 0;

                if (e->ast->index) {
                    size = *CAST(int*, e->ast->index->data);
                } else if (e->ast->value->type == AST_TYPE_LIST) {
                    size = e->ast->value->childs->size;
                }


                if (ind >= size) {
                    list_push(analyzer->errors,
                              string_format("%S:%d:%d: Error: accessing array `%S` with index larger or equals to the array size (index = %d, size = %d)",
                                ast->token->filename, ast->token->row, ast->token->col, ast->name, ind, size));
                }
            }
        } else {
            list_push(analyzer->errors,
                      string_format("%S:%d:%d: Error: undefined symbol `%S`",
                        ast->token->filename, ast->token->row, ast->token->col, ast->name));
        }
    }
    if (ast->type == AST_TYPE_RETURN && ast->value) {
        analyzer_check_var_access(analyzer, ast->value);
    }
    if (ast->type == AST_TYPE_VARIABLE && ast->value) {
        analyzer_check_var_access(analyzer, ast->value);
    }
    if (ast->type == AST_TYPE_FUNCTION) {
        LIST_FOREACH(ast->value->childs, {
            ast_t* item = node->data;
            analyzer_check_var_access(analyzer, item);
        })
    }
    if (!ast->parent || ast->parent->type != AST_TYPE_FUNCTION) {
        LIST_FOREACH(ast->childs, {
            ast_t* item = node->data;
            analyzer_check_var_access(analyzer, item);
        })
    }
    if (ast->type == AST_TYPE_CALL) {
        analyzer_check_var_access(analyzer, ast->value);
    }

}

void analyzer_check_types(analyzer_t* analyzer, ast_t* ast) {

    if (ast->type == AST_TYPE_CALL) {
        symbol_entry_t* e = symbol_table_find_entry(analyzer->symbol_table, ast->name);
        for(size_t i = 0; i < e->ast->childs->size; ++i) {
            ast_t* callVal = list_index_data(ast->value->childs, i);
            ast_t* funArg = list_index_data(e->ast->childs, i);

            if (!check_types(analyzer, funArg->data_type->name, callVal)) {
                list_push(analyzer->errors,
                          string_format("%S:%d:%d: Error: no viable conversation from `%S` to `%S`",
                            callVal->token->filename, callVal->token->row, callVal->token->col, deduce_type(analyzer, callVal), funArg->data_type->name));
            }
        }
    }
    if (ast->type == AST_TYPE_FUNCTION) {
        LIST_FOREACH(ast->value->childs, {
            ast_t* item = node->data;
            analyzer_check_types(analyzer, item);
        })
    }
    if (ast->type == AST_TYPE_VARIABLE_ARRAY && ast->value->type == AST_TYPE_LIST) {
        LIST_FOREACH(ast->value->childs, {
            ast_t* val = node->data;
            if (!check_types(analyzer, ast->data_type->name, node->data)) {
                list_push(analyzer->errors,
                          string_format("%S:%d:%d: Error: no viable conversation from `%S` to `%S`",
                            val->token->filename, val->token->row, val->token->col, deduce_type(analyzer, val), ast->data_type->name));
            }
        })
    }
    LIST_FOREACH(ast->childs, {
        ast_t* item = node->data;
        analyzer_check_types(analyzer, item);
    })

}

void analyzer_generate_warnings(analyzer_t* analyzer) {
    for(size_t i = 0; i < analyzer->symbol_table->symbols->size; ++i) {
        symbol_entry_t* entry = CAST(symbol_entry_t*, list_index_data(analyzer->symbol_table->symbols, i));
        if (!entry->is_accessed) {
            const char* t;
            if (entry->ast->type == AST_TYPE_CONSTANT)
                t = "constant";
            else if (entry->ast->type == AST_TYPE_VARIABLE || entry->ast->type == AST_TYPE_VARIABLE_ARRAY)
                t = "variable";
            else
                t = "symbol";
            list_push(analyzer->warnings,
                      string_format("%S:%d:%d: Warning: unused %s `%S`",
                      entry->file, entry->row, entry->col, t, entry->name));
        }
    }
}
