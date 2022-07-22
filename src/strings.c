#include <include/strings.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static string_t* __string_empty__ = NULL;

string_t* init_string(const string_char_type* value) {
    string_t* this = calloc(1, sizeof(struct string));
    this->size = strlen(value);
    this->buffer = calloc(this->size + 1, sizeof(string_char_type));
    strncpy(this->buffer, value, this->size);
    return this;
}

string_t* init_string_no_cpy(string_char_type* value) {
    string_t* this = calloc(1, sizeof(struct string));
    this->size = strlen(value);
    this->buffer = value;
    return this;
}

void string_free(string_t** string) {
    free((*string)->buffer);
    free(*string);
    *string = NULL;
}

string_view_t* init_string_view(string_t* string, size_t pos, size_t size) {
    string_view_t* this = calloc(1, sizeof(struct string_view));
    this->pos = pos;
    this->size = size;
    this->string = string;
    return this;
}

string_char_type* string_view_get_string(string_view_t* view) {
    string_char_type* buffer = calloc(view->size + 1, sizeof(string_char_type));
    strncpy(buffer, &view->string->buffer[view->pos], view->size);
    return buffer;
}

string_view_t* string_substr_view(string_t* string, size_t pos, size_t size) {
    return init_string_view(string, pos, size == 0 ? string->size - pos : size);
}

string_t* string_substr(string_t* string, size_t pos, size_t size) {
    string_t* this = calloc(1, sizeof(struct string));
    /*if (size == 0) {
        size = string->size - pos;
    }*/
    this->buffer = calloc(size + 1, sizeof(string_char_type));
    strncpy(this->buffer, &string->buffer[pos], size);
    this->size = size;
    this->buffer[this->size] = '\0';
    return this;
}

string_t* string_concat(string_t* lhs, string_t* rhs) {
    string_t* this = calloc(1, sizeof(struct string));
    this->buffer = calloc(lhs->size + rhs->size + 1, sizeof(string_char_type));
    this->size = lhs->size + rhs->size;
    strncpy(this->buffer, lhs->buffer, lhs->size);
    strncpy(&this->buffer[lhs->size], rhs->buffer, rhs->size);
    return this;
}

string_t* string_concat_str(string_t* lhs, const string_char_type* rhs) {
    string_t* this = calloc(1, sizeof(struct string));
    size_t len = strlen(rhs);
    this->buffer = calloc(lhs->size + len + 1, sizeof(string_char_type));
    this->size = lhs->size + len;
    strncpy(this->buffer, lhs->buffer, lhs->size);
    strncpy(&this->buffer[lhs->size], rhs, len);
    return this;
}

string_t* string_concat_view(string_t* lhs, string_view_t* rhs) {
    string_t* this = calloc(1, sizeof(struct string));
    this->buffer = calloc(lhs->size + rhs->size + 1, sizeof(string_char_type));
    this->size = lhs->size + rhs->size;
    strncpy(this->buffer, lhs->buffer, lhs->size);
    char* buffer = string_view_get_string(rhs);
    strncpy(&this->buffer[lhs->size], buffer, rhs->size);
    free(buffer);
    return this;
}

size_t string_index_of(string_t* this, const string_char_type* s, size_t pos) {
    size_t len = strlen(s);
    for(size_t i = pos; i < this->size; ++i)
        if (strncmp(&this->buffer[i], s, len) == 0)
            return i;
    return this->size + 1;
}

size_t string_last_index_of(string_t* this, const string_char_type* s) {
    size_t old = 0, ind = 0;
    do {
        ind = string_index_of(this, s, old + 1);
        if (ind < this->size) {
            old = ind;
        }
    } while (ind < this->size);
    return old;
}

bool string_cmp(string_t* lhs, string_t* rhs) {
    if (lhs == NULL || rhs == NULL)
        return false;
    return strcmp(lhs->buffer, rhs->buffer) == 0;
}

bool string_cmp_str(string_t* lhs, const string_char_type* rhs) {
    if (lhs == NULL || rhs == NULL)
        return false;
    return strcmp(lhs->buffer, rhs) == 0;
}

void string_print( string_t* string, const char* format) {
    printf(format, string->buffer);
}

string_t* string_empty() {
    if (!__string_empty__)
        __string_empty__ = init_string("");
    return __string_empty__;
}

void string_append(string_t* string, string_t* value) {
    string->buffer = realloc(string->buffer, string->size + value->size + 1);
    strncpy(&string->buffer[string->size], value->buffer, value->size);
    string->buffer[string->size + value->size] = '\0';
    string->size = string->size + value->size;
}

void string_append_str(string_t* string, const string_char_type* value) {
    size_t len = strlen(value);
    string->buffer = realloc(string->buffer, string->size + len + 1);
    strncpy(&string->buffer[string->size], value, len);
    string->buffer[string->size + len] = '\0';
    string->size = string->size + len;
}

void string_append_chr(string_t* string, string_char_type value) {
    string->buffer = realloc(string->buffer, string->size + 2);
    string->buffer[string->size] = value;
    string->buffer[string->size + 1] = '\0';
    string->size++;
}

void string_erase(string_t* string, size_t start, size_t end) {
    if (start >= string->size || end >= string->size || end - start > string->size) {
        return;
    }

    size_t len = end - start;
    string_char_type* buffer = calloc((string->size - len) + 1, sizeof(string_char_type));
    memcpy(buffer, string->buffer, start);
    memcpy(&buffer[start], &string->buffer[end], string->size - end);
    buffer[string->size - len] = '\0';
    free(string->buffer);
    string->buffer = buffer;
    string->size -= len;
}

void string_insert(string_t* string, string_t* value, size_t pos) {
    if (value == NULL || string == NULL || value->size == 0) {
        return;
    }
    size_t len = string->size + value->size;
    string_char_type* buffer = calloc(len + 1, sizeof(string_char_type));
    memcpy(buffer, string->buffer, pos);
    memcpy(&buffer[pos], value->buffer, value->size);
    memcpy(&buffer[pos + value->size], &string->buffer[pos], string->size - pos);
    buffer[len] = '\0';
    free(string->buffer);
    string->buffer = buffer;
    string->size = len;
}

void string_insert_str(string_t* string, const string_char_type* value, size_t pos) {
    size_t size = strlen(value);
    if (value == NULL || string == NULL || size == 0) {
        return;
    }
    size_t len = string->size + size;
    string_char_type* buffer = calloc(len + 1, sizeof(string_char_type));
    memcpy(buffer, string->buffer, pos);
    memcpy(&buffer[pos], value, size);
    memcpy(&buffer[pos + size], &string->buffer[pos], string->size - pos);
    buffer[len] = '\0';
    free(string->buffer);
    string->buffer = buffer;
    string->size = len;
}

string_t* string_format(const string_char_type* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    string_t* str = init_string("");
    const string_char_type* cursor = fmt;
    while (*cursor != '\0') {
        if (*cursor == '%') {
            switch (*(cursor + 1)) {
            case 'S':
            {
                string_t* S = va_arg(args, string_t*);
                string_append(str, S);
                break;
            }
            case 's':
            {
                const string_char_type* s = va_arg(args, const string_char_type*);
                string_append_str(str, s);
                break;
            }
            default:
            {
                char buf[100];
                char temp[3] = { '%', *(cursor + 1), '\0' };

                vsprintf(buf, temp, args);
                string_append_str(str, buf);
            }
            }
            cursor++;
        } else {
            string_append_chr(str, *cursor);
        }
        cursor++;
    }

    va_end(args);
    return str;
}
