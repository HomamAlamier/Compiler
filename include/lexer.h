#ifndef LEXER_H
#define LEXER_H

#include <include/list.h>
#include <include/strings.h>

enum token_type {
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_RETURN,
    TOKEN_TYPE_CONST,
    TOKEN_TYPE_EOF,

    // Seperators
    TOKEN_TYPE_LPAREN,
    TOKEN_TYPE_RPAREN,
    TOKEN_TYPE_LBRACE,
    TOKEN_TYPE_RBRACE,
    TOKEN_TYPE_LBRAKET,
    TOKEN_TYPE_RBRAKET,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_SEMI,

    // Literals
    TOKEN_TYPE_INTEGER_LITERAL,
    TOKEN_TYPE_DOUBLE_LITERAL,
    TOKEN_TYPE_NUMBER_LITERAL,
    TOKEN_TYPE_STRING_LITERAL,

    // Operators
    TOKEN_TYPE_EQUALS,
    TOKEN_TYPE_EQUALS_EQUALS,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_PLUS_EQUALS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_MINUS_EQUALS,
    TOKEN_TYPE_MUL,
    TOKEN_TYPE_MUL_EQUALS,
    TOKEN_TYPE_DIV,
    TOKEN_TYPE_DIV_EQUALS,
    TOKEN_TYPE_MOD,
    TOKEN_TYPE_MOD_EQUALS,
    TOKEN_TYPE_GT,
    TOKEN_TYPE_GT_EQUALS,
    TOKEN_TYPE_LT,
    TOKEN_TYPE_LT_EQUALS,
    TOKEN_TYPE_OPERATOR,
};


typedef struct lexer_token {
    enum token_type type;
    string_t* value;
    size_t col;
    size_t row;
    string_t* filename;
} lexer_token_t;


typedef struct lexer {
    string_t* filename;
    string_t* src;
    size_t pos;
    string_char_type ch;
    size_t col;
    size_t row;
} lexer_t;

const char* token_type_str(int type);

lexer_t* init_lexer(const char* filename, string_t* src);
lexer_token_t* init_lexer_token(string_t* value, int type, size_t col, size_t row, string_t* filename);
void lexer_free(lexer_t**);
void dump_lexer_token(lexer_token_t* token);

string_char_type lexer_peek(lexer_t* lexer, size_t offset);
void lexer_offset(lexer_t* lexer, size_t offset);

lexer_token_t* lexer_get_id(lexer_t* lexer);
lexer_token_t* lexer_get_operator(lexer_t* lexer);
lexer_token_t* lexer_get_char(lexer_t* lexer, int type);
lexer_token_t* lexer_get_number(lexer_t* lexer);
lexer_token_t* lexer_get_string(lexer_t* lexer);
lexer_token_t* lexer_get_token(lexer_t* lexer);

#endif // LEXER_H
