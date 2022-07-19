#include <include/lexer.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <include/lang_lexer_config.h>
#include <include/types.h>

static const char whitespace_char = ' ';
static const char stringquote_char = '\"';

const char* token_type_str(int type) {
    switch (type) {
    case TOKEN_TYPE_IDENTIFIER: return "TOKEN_TYPE_IDENTIFIER";
    case TOKEN_TYPE_RETURN: return "TOKEN_TYPE_RETURN";
    case TOKEN_TYPE_EOF: return "TOKEN_TYPE_EOF";

    // Seperators
    case TOKEN_TYPE_LPAREN: return "TOKEN_TYPE_LPAREN";
    case TOKEN_TYPE_RPAREN: return "TOKEN_TYPE_RPAREN";
    case TOKEN_TYPE_LBRACE: return "TOKEN_TYPE_LBRACE";
    case TOKEN_TYPE_RBRACE: return "TOKEN_TYPE_RBRACE";
    case TOKEN_TYPE_COMMA: return "TOKEN_TYPE_COMMA";
    case TOKEN_TYPE_SEMI: return "TOKEN_TYPE_SEMI";

    // Literals
    case TOKEN_TYPE_INTEGER_LITERAL: return "TOKEN_TYPE_INTEGER_LITERAL";
    case TOKEN_TYPE_DOUBLE_LITERAL: return "TOKEN_TYPE_DOUBLE_LITERAL";
    case TOKEN_TYPE_NUMBER_LITERAL: return "TOKEN_TYPE_NUMBER_LITERAL";
    case TOKEN_TYPE_STRING_LITERAL: return "TOKEN_TYPE_STRING_LITERAL";

    // Operators
    case TOKEN_TYPE_EQUALS: return "TOKEN_TYPE_EQUALS";
    case TOKEN_TYPE_EQUALS_EQUALS: return "TOKEN_TYPE_EQUALS_EQUALS";
    case TOKEN_TYPE_PLUS: return "TOKEN_TYPE_PLUS";
    case TOKEN_TYPE_PLUS_EQUALS: return "TOKEN_TYPE_PLUS_EQUALS";
    case TOKEN_TYPE_MINUS: return "TOKEN_TYPE_MINUS";
    case TOKEN_TYPE_MINUS_EQUALS: return "TOKEN_TYPE_MINUS_EQUALS";
    case TOKEN_TYPE_MUL: return "TOKEN_TYPE_MUL";
    case TOKEN_TYPE_MUL_EQUALS: return "TOKEN_TYPE_MUL_EQUALS";
    case TOKEN_TYPE_DIV: return "TOKEN_TYPE_DIV";
    case TOKEN_TYPE_DIV_EQUALS: return "TOKEN_TYPE_DIV_EQUALS";
    case TOKEN_TYPE_MOD: return "TOKEN_TYPE_MOD";
    case TOKEN_TYPE_MOD_EQUALS: return "TOKEN_TYPE_MOD_EQUALS";
    case TOKEN_TYPE_GT: return "TOKEN_TYPE_GT";
    case TOKEN_TYPE_GT_EQUALS: return "TOKEN_TYPE_GT_EQUALS";
    case TOKEN_TYPE_LT: return "TOKEN_TYPE_LT";
    case TOKEN_TYPE_LT_EQUALS: return "TOKEN_TYPE_LT_EQUALS";
    case TOKEN_TYPE_OPERATOR: return "TOKEN_TYPE_OPERATOR";
    }
    return "Invalid";
}

bool is_separator(char c) {
    for(int i = 0; i < lang_separators_size; ++i) {
        if (c == lang_separators[i]) {
            return true;
        }
    }
    return false;
}

bool is_operator_full(const char* str) {
    for(int i = 0; i < lang_operators_size; ++i) {
        if (strncmp(str, lang_operators[i], 3) == 0) {
            return true;
        }
    }
    return false;
}

bool is_operator(char ch) {
    for(int i = 0; i < lang_operators_size; ++i) {
        if (ch == lang_operators[i][0]) {
            return true;
        }
    }
    return false;
}

bool is_keyword(const char* str) {
    for(int i = 0; i < lang_keywords_size; ++i) {
        if (strcmp(str, lang_keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_number_literal(const char* str) {
    for(int i = 0; i < strnlen(str, 256); ++i) {
        bool found = false;
        for(int j = 0; j < lang_number_literal_chars_size; ++j) {
            if (str[i] == lang_number_literal_chars[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        ++str;
    }
    return true;
}

lexer_t* init_lexer(const char* filename, string_t* src) {
    lexer_t* this = calloc(1, sizeof(struct lexer));
    this->filename = init_string(filename);
    this->src = src;
    this->pos = 0;
    this->ch = *src->buffer;
    this->col = 1;
    this->row = 1;
    return this;
}

lexer_token_t* init_lexer_token(string_t* value, int type, size_t col, size_t row) {
    lexer_token_t* token = calloc(1, sizeof(struct lexer_token));
    token->value = value;
    token->type = type;
    token->col = col;
    token->row = row;
    return token;
}

void lexer_free(lexer_t** this) {
    free((*this)->src);
    free(*this);
    *this = NULL;
}

void dump_lexer_token(lexer_token_t* token) {
    printf("(type=%s, value=`%s`)\n", token_type_str(token->type), token->value->buffer);
}

string_char_type lexer_peek(lexer_t* lexer, size_t offset) {
    if (lexer->pos + offset < lexer->src->size) {
        return lexer->src->buffer[lexer->pos + offset];
    }
    return lexer->ch;
}

void lexer_offset(lexer_t* lexer, size_t offset) {
    if (lexer->pos < lexer->src->size && lexer->ch != '\0') {
        lexer->pos += offset;
        lexer->col++;
        lexer->ch = lexer->src->buffer[lexer->pos];
    }
}

lexer_token_t* lexer_get_id(lexer_t* lexer) {
    int i = lexer->pos;
    while (isalnum(lexer->ch)) {
        lexer_offset(lexer, 1);
    }
    return init_lexer_token(string_substr(lexer->src, i, lexer->pos - i), TOKEN_TYPE_IDENTIFIER, lexer->col, lexer->row);
}

lexer_token_t* lexer_get_operator(lexer_t* lexer) {
    lexer_token_t* token = init_lexer_token(NULL, TOKEN_TYPE_OPERATOR, lexer->col, lexer->row);

    switch (lexer->ch) {
    case '=': token->type = TOKEN_TYPE_EQUALS; break;
    case '+': token->type = TOKEN_TYPE_PLUS; break;
    case '-': token->type = TOKEN_TYPE_MINUS; break;
    case '*': token->type = TOKEN_TYPE_MUL; break;
    case '%': token->type = TOKEN_TYPE_MOD; break;
    case '>': token->type = TOKEN_TYPE_GT; break;
    case '<': token->type = TOKEN_TYPE_LT; break;
    }

    char str[3] = { lexer->ch };
    int nullTerm = 1;

    if (lexer_peek(lexer, 1) == '=') {
        token->type++;
        lexer_offset(lexer, 1);
        str[nullTerm] = '=';
        nullTerm++;
    }

    str[nullTerm] = '\0';

    token->value = init_string(str);

    lexer_offset(lexer, 1);
    return token;
}

lexer_token_t* lexer_get_char(lexer_t* lexer, int type) {
    lexer_token_t* token = init_lexer_token(NULL, type, lexer->col, lexer->row);
    char str[2] = { lexer->ch, '\0' };
    token->value = init_string(str);
    lexer_offset(lexer, 1);
    return token;
}

lexer_token_t* lexer_get_number(lexer_t* lexer) {
    char* buffer = calloc(256, sizeof(string_char_type));
    size_t pos = 0;
    bool is_double = false;
    while(isdigit(lexer->ch) || lexer->ch == '.') {
        buffer[pos] = lexer->ch;
        ++pos;
        lexer_offset(lexer, 1);

        if (lexer->ch == '.')
            is_double = true;
    }
    buffer[pos] = '\0';
    lexer_token_t* token = init_lexer_token(NULL, is_double ? TOKEN_TYPE_DOUBLE_LITERAL : TOKEN_TYPE_INTEGER_LITERAL, lexer->col, lexer->row);
    token->value = init_string(buffer);
    free(buffer);
    return token;
}

lexer_token_t* lexer_get_string(lexer_t* lexer) {
    lexer_offset(lexer, 1);
    size_t start = lexer->pos;
    while(lexer->ch != '\"') {
        lexer_offset(lexer, 1);
    }
    string_t* str = string_substr(lexer->src, start, (lexer->pos - start) - 1);
    lexer_offset(lexer, 1);
    return init_lexer_token(str, TOKEN_TYPE_STRING_LITERAL, lexer->col, lexer->row);
}

lexer_token_t* lexer_get_token(lexer_t* lexer) {
    while(lexer->ch != '\0') {
        if (lexer->ch == 13 || lexer->ch == '\n' || lexer->ch == ' ' || lexer->ch == '\t') {
            if (lexer->ch == '\n') {
                lexer->row++;
                lexer->col = 0;
            }
            lexer_offset(lexer, 1);
            continue;
        }

        if (isalpha(lexer->ch))
            return lexer_get_id(lexer);

        if (is_operator(lexer->ch))
            return lexer_get_operator(lexer);

        if (isdigit(lexer->ch))
            return lexer_get_number(lexer);

        switch(lexer->ch)
        {
        case '(': return lexer_get_char(lexer, TOKEN_TYPE_LPAREN);
        case ')': return lexer_get_char(lexer, TOKEN_TYPE_RPAREN);
        case '{': return lexer_get_char(lexer, TOKEN_TYPE_LBRACE);
        case '}': return lexer_get_char(lexer, TOKEN_TYPE_RBRACE);
        case ';': return lexer_get_char(lexer, TOKEN_TYPE_SEMI);
        case ',': return lexer_get_char(lexer, TOKEN_TYPE_COMMA);
        case '\"': return lexer_get_string(lexer);
        case '/': {
            char cha = lexer_peek(lexer, 1);
            if (cha == '/') {
                while(lexer->ch != '\n') {
                    lexer_offset(lexer, 1);
                }
            } else if (cha == '*') {
                while(lexer->ch != '\0') {
                    if (lexer->ch == '*' && lexer_peek(lexer, 1) == '/') {
                        lexer_offset(lexer, 2);
                        break;
                    }
                    lexer_offset(lexer, 1);
                }
            } else {
                return lexer_get_operator(lexer);
            }
            break;
        }
        }

        lexer_offset(lexer, 1);

    }
    return init_lexer_token(NULL, TOKEN_TYPE_EOF, lexer->col, lexer->row);
}
