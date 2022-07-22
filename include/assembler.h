#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <include/strings.h>
#include <include/symbol_table.h>
#include <include/ast.h>
#include <include/analyzer.h>

enum assembler_errors {
    ASSEMBLER_ERROR_UNDEFINED_SYMBOL,
    ASSEMBLER_ERROR_UNHANDLED_TYPE
};

typedef struct assembler_output {
    list_t* text;
    list_t* data;
} assembler_output_t;

typedef struct assembler {
    analyzer_t* analyzer;
    assembler_output_t* output;
    list_t* errors;
    list_t* vars_list;
} assembler_t;

assembler_t* init_assembler(analyzer_t* analyzer);

void assembler_report_and_exit(assembler_t* assembler, int error, ast_t* ast, void* data);

void assembler_generate_function(assembler_t* assembler, ast_t* ast);
void assembler_generate_data(assembler_t* assembler);
void assembler_generate(assembler_t* assembler, ast_t* ast, void* data);
bool assembler_write_output(assembler_t* assembler, const char* filename);
void dump_assembler(assembler_t* assembler);
#endif // ASSEMBLER_H
