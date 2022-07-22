#ifndef STRINGS_H
#define STRINGS_H

#include <stdlib.h>
#include <include/types.h>

typedef char string_char_type;

typedef struct string {
    string_char_type* buffer;
    size_t size;
} string_t;

typedef struct string_view {
    string_t* string;
    size_t pos;
    size_t size;
} string_view_t;


string_t* init_string(const string_char_type* value);
string_t* init_string_no_cpy(string_char_type* value);
void string_free(string_t** string);
string_view_t* init_string_view(string_t* string, size_t pos, size_t size);

string_char_type* string_view_get_string(string_view_t* view);

string_view_t* string_substr_view(string_t* string, size_t pos, size_t size);
string_t* string_substr(string_t* string, size_t pos, size_t size);

string_t* string_concat(string_t* lhs, string_t* rhs);
string_t* string_concat_str(string_t* lhs, const string_char_type* rhs);
string_t* string_concat_view(string_t* lhs, string_view_t* rhs);

size_t string_index_of(string_t* this, const string_char_type* s, size_t pos);
size_t string_last_index_of(string_t* this, const string_char_type* s);

bool string_cmp(string_t* lhs, string_t* rhs);
bool string_cmp_str(string_t* lhs, const string_char_type* rhs);

void string_print(string_t* string, const char* format);

string_t* string_empty();

void string_append(string_t* string, string_t* value);
void string_append_str(string_t* string, const string_char_type* value);
void string_append_chr(string_t* string, string_char_type value);

void string_erase(string_t* string, size_t start, size_t end);
void string_insert(string_t* string, string_t* value, size_t pos);
void string_insert_str(string_t* string, const string_char_type* value, size_t pos);

string_t* string_format(const string_char_type* fmt, ...);
#endif // STRINGS_H
