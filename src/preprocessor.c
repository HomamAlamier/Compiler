#include <include/preprocessor.h>
#include <include/io.h>
#include <include/log.h>
#include <include/path.h>
#include <ctype.h>

#define BACKUP_PREPROCESSOR_POS(PRE) \
    size_t __pre_pos = PRE->pos; \
    size_t __pre_row = PRE->row; \
    size_t __pre_col = PRE->col; \
    string_char_type __pre_ch = PRE->ch

#define RESTORE_PREPROCESSOR_POS(PRE) \
    PRE->pos = __pre_pos; \
    PRE->row = __pre_row; \
    PRE->col = __pre_col; \
    PRE->ch = __pre_ch

#define DEBUG_STRING(STR) \
    const char* debug__##STR = STR->buffer

LOG_TAG("Preprocessor");

enum preprocessor_errors {
    PREPROCESSOR_ERROR_UNEXPECTED_CHAR,
    PREPROCESSOR_ERROR_CANNOT_OPEN_INCLUDE_FILE
};

string_t* read_name(preprocessor_t* preprocessor) {
    size_t st = preprocessor->pos;
    while(isalnum(preprocessor->ch) || preprocessor->ch == '_' || preprocessor->ch == ' ') {
        preprocessor_offset(preprocessor, 1);
    }
    string_t* s = string_substr(preprocessor->src, st, preprocessor->pos - st);
    DEBUG_STRING(s);
    return string_trim(s);
}

bool is_num(string_t* str) {
    for(int i = 0; i < str->size; ++i)
        if (!isdigit(str->buffer[i]))
            return false;
    return true;
}

bool eval_if(preprocessor_t* preprocessor) {
    string_t* name = read_name(preprocessor);
    DEBUG_STRING(name);
    bool result = false;
    if (string_cmp_str(name, "defined")) {
        preprocessor_poll(preprocessor, '(');
        string_t* val = read_name(preprocessor);
        DEBUG_STRING(val);
        preprocessor_poll(preprocessor, ')');
        result = preprocessor_is_defined(preprocessor, val);
    } else if (preprocessor->ch == '=') {
        preprocessor_poll(preprocessor, '=');
        preprocessor_poll(preprocessor, '=');
        string_t* val = read_name(preprocessor);
        DEBUG_STRING(val);
        DEBUG_STRING(name);
        define_entry_t* e = preprocessor_find_define(preprocessor, name);
        if (e) {
            const char* debug = e->value->buffer;
            result = string_cmp(val, e->value);
        }
        else
            result = string_cmp(val, name);;
    } else if (is_num(name)) {
        int x = atoi(name->buffer);
        result = (x != 0);
    }

    if (preprocessor->ch == '&') {
        preprocessor_poll(preprocessor, '&');
        preprocessor_poll(preprocessor, '&');
        return result && eval_if(preprocessor);
    }
    return result;
}

void preprocessor_report_and_exit(preprocessor_t* preprocessor, int error, char* data) {
    switch (error) {
    case PREPROCESSOR_ERROR_UNEXPECTED_CHAR:
        LOG_PUSH_ARGS("Error: Unexpected char `%c` (expected `%s`)", preprocessor->ch, data);
        break;
    case PREPROCESSOR_ERROR_CANNOT_OPEN_INCLUDE_FILE:
        LOG_PUSH_ARGS("Error: cannot find include file `%s`", data);
        break;
    }
    exit(1);
}

define_entry_t* init_define_entry(string_t* name, string_t* value) {
    define_entry_t* this = calloc(1, sizeof(struct define_entry));
    this->name = name;
    this->value = value;
    return this;
}

preprocessor_t* init_preprocessor(string_t* filename, string_t* src) {
    preprocessor_t* this = calloc(1, sizeof(struct preprocessor));
    this->included_files = init_list();
    this->define_list = init_list();
    this->row = 0;
    this->col = 0;
    this->pos = 0;
    this->linesRemoved = 0;
    this->ch = *src->buffer;
    this->src = src;
    this->filename = filename;
    return this;
}

include_file_t* init_include_file(string_t* filename, size_t begin, size_t end) {
    include_file_t* this = calloc(1, sizeof(struct include_file));
    this->filename = filename;
    this->begin = begin;
    this->end = end;
    return this;
}

void preprocessor_offset(preprocessor_t* preprocessor, size_t offset) {
    if (preprocessor->pos + offset > preprocessor->src->size) {
        return;
    }
    preprocessor->pos += offset;
    preprocessor->col += offset;
    preprocessor->ch = preprocessor->src->buffer[preprocessor->pos];
}

void preprocessor_poll(preprocessor_t* preprocessor, string_char_type ch) {
    if (preprocessor->ch != ch) {
        char z[2] = { ch, '\0' };
        preprocessor_report_and_exit(preprocessor, PREPROCESSOR_ERROR_UNEXPECTED_CHAR, z);
    }
    preprocessor_offset(preprocessor, 1);
}

void preprocessor_include(preprocessor_t* preprocessor, size_t start) {
    preprocessor_poll(preprocessor, '(');
    preprocessor_poll(preprocessor, '\"');


    size_t st = preprocessor->pos;

    while(preprocessor->ch != '\"' && preprocessor->ch != '\0') {
        preprocessor_offset(preprocessor, 1);
    }

    string_t* file = string_substr(preprocessor->src, st, preprocessor->pos - st);
    string_t* path = path_make_path(path_get_dir(preprocessor->filename), file);
    string_free(&file);

    include_file_t* include_file = init_include_file(path, preprocessor->row, 0);
    list_push(preprocessor->included_files, include_file);

    preprocessor_offset(preprocessor, 1);

    preprocessor_poll(preprocessor, ')');


    string_erase(preprocessor->src, start, preprocessor->pos);

    char* includeFile = read_file_full(path->buffer);

    if (includeFile == NULL) {
        preprocessor_report_and_exit(preprocessor, PREPROCESSOR_ERROR_CANNOT_OPEN_INCLUDE_FILE, path->buffer);
    }

    string_t* src = init_string_no_cpy(includeFile);

    preprocessor_t* pre = init_preprocessor(path, src);
    preprocessor_preprocess(pre);

    include_file->end = preprocessor->row + pre->row;

    list_append_list(preprocessor->define_list, pre->define_list);
    list_append_list(preprocessor->included_files, pre->included_files);

    list_free(&pre->define_list, false);
    list_free(&pre->included_files, false);

    free(pre);

    string_insert(preprocessor->src, src, start);
    preprocessor->row = src->size;
    preprocessor_offset(preprocessor, -(preprocessor->pos - start));
}

void preprocessor_define(preprocessor_t* preprocessor, size_t start) {
    preprocessor_poll(preprocessor, '(');

    string_t* name = read_name(preprocessor);
    string_t* value = string_empty();

    const char* debug = name->buffer;

    if (preprocessor->ch == ',') {
        preprocessor_offset(preprocessor, 1);
        value = read_name(preprocessor);
    }
    const char* debug2 = value->buffer;

    preprocessor_poll(preprocessor, ')');


    list_push(preprocessor->define_list, init_define_entry(name, value));
    string_erase(preprocessor->src, start, preprocessor->pos + 1);
    preprocessor->linesRemoved--;

    preprocessor_offset(preprocessor, -(preprocessor->pos + 1 - start));


    if (value->size != 0) {
        size_t ind = 0;
        do {
            ind = string_index_of(preprocessor->src, name, 0);
            if (ind < preprocessor->src->size) {
                string_erase(preprocessor->src, ind, ind + name->size);
                string_insert(preprocessor->src, value, ind);
            }
        } while (ind < preprocessor->src->size);
    }
}

void preprocessor_if(preprocessor_t* preprocessor, size_t start) {

    BACKUP_PREPROCESSOR_POS(preprocessor);
    const char* src = &preprocessor->src->buffer[preprocessor->pos];
    preprocessor_poll(preprocessor, '(');
    bool result = eval_if(preprocessor);
    preprocessor_poll(preprocessor, ')');
    int if_nest = 0;
    size_t blockStart = preprocessor->pos;
    while(preprocessor->ch != '\0') {
        if (preprocessor->ch == '@') {
            preprocessor_offset(preprocessor, 1);
            string_t* n = read_name(preprocessor);
            if (string_cmp_str(n, "if")) {
                if_nest++;
            } else if (string_cmp_str(n, "endif")) {
                if_nest--;
            }
            if (if_nest < 0)
                break;
        }

        preprocessor_offset(preprocessor, 1);

    }

    string_t* block = string_substr(preprocessor->src, blockStart, preprocessor->pos - blockStart);

    DEBUG_STRING(block);
    size_t ind = string_index_of_str(block, "@endif", 0);
    string_erase(block, ind, ind + 6);
    string_erase(preprocessor->src, start, preprocessor->pos + 1);


    debug__block = block->buffer;
    if (result)
        string_insert(preprocessor->src, block, start);
    else
        string_insert_str(preprocessor->src, "// FALSE", start);

    RESTORE_PREPROCESSOR_POS(preprocessor);
    preprocessor->pos = start;
    // TODO: implement this
}

void preprocessor_preprocess(preprocessor_t* preprocessor) {
    while(preprocessor->ch != '\0') {
        if (preprocessor->ch == '\n') {
            preprocessor->row++;
            preprocessor->col = 0;
        }
        if (preprocessor->ch == '@') {
            preprocessor_offset(preprocessor, 1);
            size_t start = preprocessor->pos;
            while(preprocessor->ch != '\n') {
                if (!isalpha(preprocessor->ch)) {
                    string_t* word = string_substr(preprocessor->src, start, preprocessor->pos - start);
                    const char* debug = word->buffer;
                    if (string_cmp_str(word, "include")) {
                        preprocessor_include(preprocessor, start - 1);
                        break;
                    } else if (string_cmp_str(word, "define")) {
                        preprocessor_define(preprocessor, start - 1);
                        break;
                    } else if (string_cmp_str(word, "if")) {
                        preprocessor_if(preprocessor, start - 1);
                    }
                }
                preprocessor_offset(preprocessor, 1);
            }

        }

        preprocessor_offset(preprocessor, 1);
    }

}

bool preprocessor_is_defined(preprocessor_t* preprocessor, string_t* name) {
    LIST_FOREACH(preprocessor->define_list, {
        define_entry_t* item = node->data;
        if (string_cmp(item->name, name)) {
            return true;
        }
    })
    return false;
}

define_entry_t* preprocessor_find_define(preprocessor_t* preprocessor, string_t* name) {
    LIST_FOREACH(preprocessor->define_list, {
        define_entry_t* item = node->data;
        if (string_cmp(item->name, name)) {
            return item;
        }
    })
    return NULL;
}
