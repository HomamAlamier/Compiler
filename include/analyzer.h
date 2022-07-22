#ifndef ANALYZER_H
#define ANALYZER_H

#include <include/symbol_table.h>
#include <include/ast.h>

typedef struct analyzer {
    symbol_table_t* symbol_table;
    list_t* errors;
} analyzer_t;


analyzer_t* init_analyzer();

void analyzer_report_and_exit(analyzer_t* analyzer);
void analyzer_analyze(analyzer_t* analyzer, ast_t* ast);
void analyzer_check_redefinetion(analyzer_t* analyzer);
void analyzer_check_calls(analyzer_t* analyzer, ast_t* ast);
void analyzer_check_types(analyzer_t* analyzer, ast_t* ast);


#endif // ANALYZER_H
