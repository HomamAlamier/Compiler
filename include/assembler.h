#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <include/strings.h>
#include <include/symbol_table.h>
#include <include/ast.h>
#include <include/analyzer.h>

enum assembler_errors {
    ASSEMBLER_ERROR_SYMBOL_REDEFINITION
};

typedef struct assembler {
    analyzer_t* analyzer;
    list_t* output;
    list_t* errors;
} assembler_t;

assembler_t* init_assembler(analyzer_t* analyzer);

void assembler_report_and_exit(assembler_t* assembler, int error, void** data);
void assembler_generate_function(assembler_t* assembler, ast_t* ast, list_t* vars);
void assembler_generate(assembler_t* assembler, ast_t* ast, void* data);

#endif // ASSEMBLER_H
