#ifndef PARSER_H
#define PARSER_H

#include <include/lexer.h>
#include <include/ast.h>

enum parser_errors {
    PARSER_ERROR_UNEXPECTED_TOKEN,
    PARSER_ERROR_UNHANDLED_EXPER,
};

typedef struct parser {
    lexer_t* lexer;
    lexer_token_t* token;
} parser_t;


parser_t* init_parser(lexer_t* lexer);
void parser_poll(parser_t* parser, int type);
void parser_report_and_exit(parser_t* parser, int error, void* data);

ast_t* parser_parse_arglist(parser_t* parser);
ast_t* parser_parse_template(parser_t* parser);
ast_t* parser_parse_integer(parser_t* parser);
ast_t* parser_parse_double(parser_t* parser);
ast_t* parser_parse_string(parser_t* parser);

ast_t* parser_parse_expr(parser_t* parser);
ast_t* parser_parse_assign(parser_t* parser);
ast_t* parser_parse_arth(parser_t* parser);
ast_t* parser_parse_return(parser_t* parser);
ast_t* parser_parse_id(parser_t* parser);
ast_t* parser_parse_block(parser_t* parser);

#endif // PARSER_H
