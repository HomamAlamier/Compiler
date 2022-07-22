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
    string_char_type ch;
    string_t* src;
    string_t* filename;
} preprocessor_t;

preprocessor_t* init_preprocessor(string_t* filename, string_t* src);

void preprocesser_offset(preprocessor_t* preprocessor, size_t offset);
void preprocesser_include(preprocessor_t* preprocessor, size_t start);
void preprocesser_preprocess(preprocessor_t* preprocessor);
#endif // PREPROCESSOR_H
