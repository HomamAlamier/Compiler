#include <include/preprocessor.h>
#include <include/io.h>
#include <include/log.h>
#include <include/path.h>
#include <ctype.h>

LOG_TAG("Preprocessor");

enum preprocessor_errors {
    PREPROCESSOR_ERROR_UNEXPECTED_CHAR,
    PREPROCESSOR_ERROR_CANNOT_OPEN_INCLUDE_FILE
};

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

preprocessor_t* init_preprocessor(string_t* filename, string_t* src) {
    preprocessor_t* this = calloc(1, sizeof(struct preprocessor));
    this->included_files = init_list();
    this->define_list = init_list();
    this->row = 0;
    this->col = 0;
    this->pos = 0;
    this->ch = *src->buffer;
    this->src = src;
    this->filename = filename;
    return this;
}


void preprocesser_offset(preprocessor_t* preprocessor, size_t offset) {
    if (preprocessor->pos + offset > preprocessor->src->size) {
        return;
    }
    preprocessor->pos += offset;
    preprocessor->col += offset;
    preprocessor->ch = preprocessor->src->buffer[preprocessor->pos];
}

void preprocesser_include(preprocessor_t* preprocessor, size_t start) {
    if (preprocessor->ch != '(') {
        preprocessor_report_and_exit(preprocessor, PREPROCESSOR_ERROR_UNEXPECTED_CHAR, "(");
    }

    preprocesser_offset(preprocessor, 1);

    if (preprocessor->ch != '\"') {
        preprocessor_report_and_exit(preprocessor, PREPROCESSOR_ERROR_UNEXPECTED_CHAR, "\"");
    }

    preprocesser_offset(preprocessor, 1);
    size_t st = preprocessor->pos;

    while(preprocessor->ch != '\"' && preprocessor->ch != '\0') {
        preprocesser_offset(preprocessor, 1);
    }

    string_t* file = string_substr(preprocessor->src, st, preprocessor->pos - st);
    string_t* path = path_make_path(path_get_dir(preprocessor->filename), file);
    string_free(&file);


    list_push(preprocessor->included_files, path);

    preprocesser_offset(preprocessor, 1);
    if (preprocessor->ch != ')') {
        preprocessor_report_and_exit(preprocessor, PREPROCESSOR_ERROR_UNEXPECTED_CHAR, ")");
    }


    string_erase(preprocessor->src, start, start + 12 + path->size);

    char* includeFile = read_file_full(path->buffer);

    if (includeFile == NULL) {
        preprocessor_report_and_exit(preprocessor, PREPROCESSOR_ERROR_CANNOT_OPEN_INCLUDE_FILE, path->buffer);
    }

    string_t* src = init_string_no_cpy(includeFile);

    preprocessor_t* pre = init_preprocessor(path, src);
    preprocesser_preprocess(pre);

    list_append_list(preprocessor->define_list, pre->define_list);
    list_append_list(preprocessor->included_files, pre->included_files);

    list_free(&pre->define_list, false);
    list_free(&pre->included_files, false);

    free(pre);

    string_insert(preprocessor->src, src, start);
}

void preprocesser_preprocess(preprocessor_t* preprocessor) {
    while(preprocessor->ch != '\0') {
        if (preprocessor->ch == '\n') {
            preprocessor->row++;
            preprocessor->col = 0;
        }
        if (preprocessor->ch == '@') {
            preprocesser_offset(preprocessor, 1);
            size_t start = preprocessor->pos;
            while(preprocessor->ch != '\n') {
                if (!isalpha(preprocessor->ch)) {
                    string_t* word = string_substr(preprocessor->src, start, preprocessor->pos - start);
                    if (string_cmp_str(word, "include")) {
                        preprocesser_include(preprocessor, start - 1);
                    }
                }
                preprocesser_offset(preprocessor, 1);
            }

        }

        preprocesser_offset(preprocessor, 1);
    }

}

