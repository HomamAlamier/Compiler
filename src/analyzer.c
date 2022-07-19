#include <include/analyzer.h>
#include <stdlib.h>
#include <include/strings.h>
#include <include/types.h>
#include <include/log.h>

LOG_TAG("Analyzer");

string_t* gen_func_sig(ast_t* root) {
    if (root->type == AST_TYPE_FUNCTION) {
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
    if (string_cmp(e1->name, e2->name) == STRING_COMPARE_RESULT_OK) {
        return true;
    } else {
        return string_cmp(e1->sig, e2->sig) == STRING_COMPARE_RESULT_OK;
    }
}


analyzer_t* init_analyzer() {
    analyzer_t* this = calloc(1, sizeof(struct analyzer));
    this->symbol_table = init_symbol_table();
    this->errors = init_list();
    return this;
}

void analuzer_report_and_exit(analyzer_t* analyzer) {
    if (analyzer->errors->size) {
        LIST_FOREACH(analyzer->errors, {
            LOG_PUSH(CAST(string_t*, node->data)->buffer);
        })
        exit(1);
    }
}

void analyzer_analyze_internal(analyzer_t* analyzer, ast_t* ast) {
    if (ast->type == AST_TYPE_FUNCTION) {



        list_t* data = CAST(list_t*, ast->data);

        list_push(analyzer->symbol_table->symbols,
                  init_symbol_entry(ast->data_type->name, ast->name, gen_func_sig(ast),
                                    CAST(string_t*, list_index_data(data, 0)),
                                    *CAST(size_t*, list_index_data(data, 1)),
                                    *CAST(size_t*, list_index_data(data, 2))));

    }

    LIST_FOREACH(ast->childs, {
        ast_t* tree = CAST(ast_t*, node->data);
        analyzer_analyze_internal(analyzer, tree);
    });
}

void analyzer_analyze(analyzer_t* analyzer, ast_t* ast) {
    analyzer_analyze_internal(analyzer, ast);
    analyzer_check(analyzer);
    analuzer_report_and_exit(analyzer);
}

void analyzer_check(analyzer_t* analyzer) {
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
