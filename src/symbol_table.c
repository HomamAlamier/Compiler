#include <include/symbol_table.h>
#include <stdlib.h>
#include <stdio.h>

symbol_table_t* init_symbol_table() {
    symbol_table_t* this = calloc(1, sizeof(struct symbol_table));
    this->symbols = init_list();
    return this;
}

symbol_entry_t* init_symbol_entry(string_t* type, string_t* name, string_t* sig,
                                  string_t* file, size_t col, size_t row) {
    symbol_entry_t* this = calloc(1, sizeof(struct symbol_entry));
    this->type = type;
    this->name = name;
    this->sig = sig;
    this->file = file;
    this->col = col;
    this->row = row;
    return this;
}

void dump_symbol_entry(symbol_entry_t* this) {
    printf("---> symbol_entry <---\n"
           "type: %s\n"
           "name: %s\n"
           "sig: %s\n"
           "file: %s\n"
           "col: %lu\n"
           "row: %lu\n",
           this->type->buffer, this->name->buffer, this->sig->buffer, this->file->buffer, this->col, this->row);
}
