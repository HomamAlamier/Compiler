#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <include/strings.h>
#include <include/list.h>
#include <include/types.h>

typedef struct preprocessor {
    list_t* included_files;
    list_t* define_list;
    size_t row;
    size_t col;
    size_t pos;
    size_t linesRemoved;
    string_char_type ch;
    string_t* src;
    string_t* filename;
} preprocessor_t;

typedef struct include_file {
    string_t* filename;
    size_t begin;
    size_t end;
} include_file_t;

typedef struct define_entry {
    string_t* name;
    string_t* value;
} define_entry_t;

define_entry_t* init_define_entry(string_t* name, string_t* value);

preprocessor_t* init_preprocessor(string_t* filename, string_t* src);

void preprocessor_offset(preprocessor_t* preprocessor, size_t offset);
void preprocessor_poll(preprocessor_t* preprocessor, string_char_type ch);
void preprocessor_include(preprocessor_t* preprocessor, size_t start);
void preprocessor_define(preprocessor_t* preprocessor, size_t start);
void preprocessor_if(preprocessor_t* preprocessor, size_t start);
void preprocessor_preprocess(preprocessor_t* preprocessor);

bool preprocessor_is_defined(preprocessor_t* preprocessor, string_t* name);
define_entry_t* preprocessor_find_define(preprocessor_t* preprocessor, string_t* name);
#endif // PREPROCESSOR_H
