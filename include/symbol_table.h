#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <include/strings.h>
#include <include/list.h>

typedef struct symbol_entry {
    string_t* type;
    string_t* name;
    string_t* sig;
    string_t* file;
    size_t col;
    size_t row;
} symbol_entry_t;

typedef struct symbol_table {
    list_t* symbols;
} symbol_table_t;

symbol_table_t* init_symbol_table();
symbol_entry_t* init_symbol_entry(string_t* type, string_t* name, string_t* sig,
                                  string_t* file, size_t col, size_t row);

void dump_symbol_entry(symbol_entry_t*);
#endif // SYMBOL_TABLE_H
