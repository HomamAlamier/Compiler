#include <include/parser.h>
#include <include/log.h>
#include <include/types.h>
#include <stdio.h>
#include <memory.h>

LOG_TAG("Parser");

bool token_is_operator(int type)
{
    if (type >= TOKEN_TYPE_EQUALS && type <= TOKEN_TYPE_OPERATOR)
        return true;
    return false;
}

parser_t* init_parser(lexer_t* lexer) {
    parser_t* parser = calloc(1, sizeof(struct parser));
    parser->lexer = lexer;
    parser->token = lexer_get_token(lexer);
    return parser;
}

int* heap_int(int value) {
    void* mem = calloc(1, sizeof(int));
    memcpy(mem, &value, sizeof(int));
    return mem;
}

double* heap_double(double value) {
    void* mem = calloc(1, sizeof(double));
    memcpy(mem, &value, sizeof(double));
    return mem;
}

double* heap_size_t(size_t value) {
    void* mem = calloc(1, sizeof(size_t));
    memcpy(mem, &value, sizeof(size_t));
    return mem;
}

void parser_poll(parser_t* parser, int type) {
    if (parser->token->type != type) {
        parser_report_and_exit(parser, PARSER_ERROR_UNEXPECTED_TOKEN, &type);
    }

    parser->token = lexer_get_token(parser->lexer);
}

inline void parser_report_and_exit(parser_t* parser, int error, void* data) {
    switch (error) {
    case PARSER_ERROR_UNEXPECTED_TOKEN:
        LOG_PUSH_ARGS("%s:(%d:%d): Unexpected token %s (expected %s)",
                 parser->lexer->filename->buffer, parser->lexer->row, parser->lexer->col - 1,
                 token_type_str(parser->token->type), token_type_str(*(int*)data));
        break;
    case PARSER_ERROR_UNHANDLED_EXPER:
        LOG_PUSH_ARGS("%s:(%d:%d): Unhandled experssion %s (value = `%s`)",
                 parser->lexer->filename->buffer, parser->lexer->row, parser->lexer->col - 1,
                 token_type_str(parser->token->type), parser->token->value->buffer);
        break;
    }
    exit(1);
}

ast_t* parser_parse_arglist(parser_t* parser) {
    parser_poll(parser, TOKEN_TYPE_LPAREN);

    ast_t* tree = init_ast(AST_TYPE_COMPOUND);

    if (parser->token->type != TOKEN_TYPE_RPAREN) {

        list_push(tree->childs, parser_parse_expr(parser));


        if (CAST(ast_t*, tree->childs->bottom->data)->type ==  AST_TYPE_COMPOUND) {
            LOG_PUSH_ARGS("found args (name = %s, type = %s)",
                          CAST(ast_t*, tree->childs->bottom->data)->name->buffer,
                          CAST(ast_t*, tree->childs->bottom->data)->data_type->name->buffer);
        } else {
            LOG_PUSH_ARGS("found args (name = %s, type = %s)",
                          CAST(ast_t*, tree->childs->bottom->data)->name->buffer,
                          CAST(ast_t*, tree->childs->bottom->data)->data);
        }

        while(parser->token->type == TOKEN_TYPE_COMMA) {
            parser_poll(parser, TOKEN_TYPE_COMMA);
            list_push(tree->childs, parser_parse_expr(parser));
            LOG_PUSH_ARGS("found args (name = %s, type = %s)",
                          CAST(ast_t*, tree->childs->bottom->data)->name->buffer,
                          CAST(ast_t*, tree->childs->bottom->data)->data_type->name->buffer);    }
    }

    parser_poll(parser, TOKEN_TYPE_RPAREN);

    return tree;
}

ast_t* parser_parse_template(parser_t* parser) {

    parser_poll(parser, TOKEN_TYPE_LT);

    ast_t* tree = init_ast(AST_TYPE_TEMPLATE);

    list_push(tree->childs, parser_parse_expr(parser));

    while(parser->token->type == TOKEN_TYPE_COMMA) {
        parser_poll(parser, TOKEN_TYPE_COMMA);
        list_push(tree->childs, parser_parse_expr(parser));
    }


    parser_poll(parser, TOKEN_TYPE_GT);

    return tree;
}

ast_t* parser_parse_integer(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_INTEGER);
    tree->data = heap_int(atoi(parser->token->value->buffer));
    parser_poll(parser, TOKEN_TYPE_INTEGER_LITERAL);
    return tree;
}

ast_t* parser_parse_double(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_DOUBLE);
    tree->data = heap_double(atof(parser->token->value->buffer));
    parser_poll(parser, TOKEN_TYPE_DOUBLE_LITERAL);
    return tree;
}

ast_t* parser_parse_string(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_STRING);
    tree->name = init_string("string");
    tree->data = parser->token->value;
    parser_poll(parser, TOKEN_TYPE_STRING_LITERAL);
    return tree;
}

ast_t* parser_parse_id(parser_t* parser) {
    if (parser->token->type == TOKEN_TYPE_IDENTIFIER) {

        string_t* id = parser->token->value;
        parser_poll(parser, TOKEN_TYPE_IDENTIFIER);

        if (string_cmp_str(id, "return") == STRING_COMPARE_RESULT_OK) {
            ast_t* tree = init_ast(AST_TYPE_RETURN);
            tree->name = id;

            tree->value = parser_parse_arth(parser);

            parser_poll(parser, TOKEN_TYPE_SEMI);

            return tree;
        }


        if (token_is_operator(parser->token->type)) {

            if (parser->token->type == TOKEN_TYPE_EQUALS) {
                ast_t* tree = init_ast(AST_TYPE_ASSIGNMENT);
                ast_t* lhs = init_ast(AST_TYPE_ACCESS);
                lhs->name = id;
                list_push(tree->childs, lhs);
                list_push(tree->childs, parser_parse_arth(parser));
                return tree;
            }

        } else if (parser->token->type == TOKEN_TYPE_IDENTIFIER) {
            LOG_PUSH_ARGS("possible decleration ! (type: %s, name: %s)", id->buffer, parser->token->value->buffer);

            ast_t* tree = init_ast(AST_TYPE_VARIABLE);
            tree->name = parser->token->value;
            tree->data_type = init_complex_data_type(id);

            parser_poll(parser, TOKEN_TYPE_IDENTIFIER);

            switch (parser->token->type) {
            case TOKEN_TYPE_EQUALS:
                parser_poll(parser, TOKEN_TYPE_EQUALS);
                tree->value = parser_parse_arth(parser);
                parser_poll(parser, TOKEN_TYPE_SEMI);
                break;
            case TOKEN_TYPE_SEMI:
            case TOKEN_TYPE_COMMA:
            case TOKEN_TYPE_RPAREN:
                LOG_PUSH("arg or variable decleration !");
                break;
            case TOKEN_TYPE_LPAREN:
            {
                LOG_PUSH("Function !");
                free(tree->childs);
                tree->type = AST_TYPE_FUNCTION;


                ast_t* args = parser_parse_arglist(parser);
                tree->childs = args->childs;
                args->childs = NULL;
                free(args);

                tree->data = init_list();
                list_push(tree->data, parser->lexer->filename);
                list_push(tree->data, heap_size_t(parser->lexer->col));
                list_push(tree->data, heap_size_t(parser->lexer->row));

                switch (parser->token->type) {
                case TOKEN_TYPE_LBRACE:
                    tree->value = parser_parse_expr(parser);
                    break;
                case TOKEN_TYPE_SEMI:
                    parser_poll(parser, TOKEN_TYPE_SEMI);
                    LOG_PUSH_ARGS("Function `%s` has no body !", tree->name->buffer);
                    break;
                default:
                    parser_report_and_exit(parser, PARSER_ERROR_UNHANDLED_EXPER, NULL);
                }

                break;
            }
            default:
                parser_report_and_exit(parser, PARSER_ERROR_UNHANDLED_EXPER, NULL);
            }

            return tree;
        } else if (parser->token->type == TOKEN_TYPE_LPAREN) {
            ast_t* tree = init_ast(AST_TYPE_CALL);
            tree->name = id;
            tree->value = parser_parse_arglist(parser);
            parser_poll(parser, TOKEN_TYPE_SEMI);
            return tree;
        }

    }
    return NULL;
}

ast_t* parser_parse_expr(parser_t* parser) {

    LOG_PUSH_ARGS("Token (type: %s, value: %s)", token_type_str(parser->token->type), parser->token->value->buffer);


    switch(parser->token->type) {
    case TOKEN_TYPE_IDENTIFIER: return parser_parse_id(parser);
    case TOKEN_TYPE_EQUALS: return parser_parse_arth(parser);
    case TOKEN_TYPE_LBRACE: return parser_parse_block(parser);
    case TOKEN_TYPE_INTEGER_LITERAL: return parser_parse_integer(parser);
    case TOKEN_TYPE_DOUBLE_LITERAL: return parser_parse_double(parser);
    case TOKEN_TYPE_STRING_LITERAL: return parser_parse_string(parser);
    default:
    {
        ast_t* tree = init_ast(AST_TYPE_STATEMENT);
        tree->name = parser->token->value;
        parser_poll(parser, parser->token->type);
        return tree;
    }
    }

}

ast_t* parser_parse_arth(parser_t* parser) {

    ast_t* tree = NULL;


    switch (parser->token->type) {
    case TOKEN_TYPE_INTEGER_LITERAL:
        tree = parser_parse_integer(parser);
        break;
    case TOKEN_TYPE_DOUBLE_LITERAL:
        tree = parser_parse_double(parser);
        break;
    case TOKEN_TYPE_IDENTIFIER:
        tree = init_ast(AST_TYPE_ACCESS);
        tree->name = parser->token->value;
        parser_poll(parser, TOKEN_TYPE_IDENTIFIER);
        break;
    default:
        parser_report_and_exit(parser, PARSER_ERROR_UNHANDLED_EXPER, NULL);
    }

    if (token_is_operator(parser->token->type)) {
        ast_t* tmp = init_ast(AST_TYPE_OPERATION);
        tmp->name = parser->token->value;
        list_push(tmp->childs, tree);
        parser_poll(parser, parser->token->type);
        list_push(tmp->childs, parser_parse_arth(parser));
        tree = tmp;
    }

    return tree;
}

ast_t* parser_parse_assign(parser_t* parser) {

    ast_t* tree = init_ast(AST_TYPE_VARIABLE);
    parser_poll(parser, parser->token->type);

    tree->name = parser->token->value;

    if (parser->token->type == TOKEN_TYPE_OPERATOR) {
        ast_t* var = init_ast(AST_TYPE_VARIABLE);
        var->name = tree->name;
        tree->name = parser->token->value;
        tree->type = AST_TYPE_OPERATION;
        list_push(tree->childs, var);
        list_push(tree->childs, parser_parse_assign(parser));
    }

    parser_poll(parser, parser->token->type);
    parser_poll(parser, TOKEN_TYPE_SEMI);

    return tree;
}

ast_t* parser_parse_return(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_RETURN);
    parser_poll(parser, TOKEN_TYPE_IDENTIFIER);

    tree->name = init_string("return");
    tree->value = parser_parse_expr(parser);

    parser_poll(parser, TOKEN_TYPE_SEMI);
    return tree;
}
/*
ast_t* parser_parse_id(parser_t* parser) {

    if (string_cmp_str(parser->token->value, "return") == STRING_COMPARE_RESULT_OK) {
        return parser_parse_return(parser);
    }

    ast_t* tree = init_ast(AST_TYPE_VARIABLE);
    string_t* value = parser->token->value;

    parser_poll(parser, TOKEN_TYPE_IDENTIFIER);

    printf("poll `%s'\n", parser->token->value->buffer);

    if (parser->token->type == TOKEN_TYPE_IDENTIFIER) {
        LOG_PUSH("Found possible decleration !");

        tree->data_type = init_complex_data_type(value);
        tree->name = parser->token->value;
        parser_poll(parser, TOKEN_TYPE_IDENTIFIER);


        switch(parser->token->type)
        {
        case TOKEN_TYPE_EQUALS:
        {
            LOG_PUSH("Found variable decl with assigment !");
            tree->value = parser_parse_expr(parser);
            return tree;
            break;
        }
        case TOKEN_TYPE_COMMA:
        case TOKEN_TYPE_SEMI:
            LOG_PUSH("Found variable decl !");
            return tree;
            break;
        case TOKEN_TYPE_LPAREN:
            LOG_PUSH("Found function decl !");
            tree->type = AST_TYPE_FUNCTION;
            tree->value = parser_parse_arglist(parser);
            return tree;
            break;
        default:
        {
            ast_t* val = init_ast(AST_TYPE_STATEMENT);
            val->name = parser->token->value;
            parser_poll(parser, parser->token->type);
            tree->value = val;
        }
        }

    } else if (parser->token->type == TOKEN_TYPE_LT) {
        tree->data_type->template = parser_parse_template(parser);
        parser_poll(parser, TOKEN_TYPE_IDENTIFIER);
    } else if (token_is_operator(parser->token->type)) {

    }

    return tree;
}
*/
ast_t* parser_parse_block(parser_t* parser) {
    parser_poll(parser, TOKEN_TYPE_LBRACE);

    ast_t* tree = init_ast(AST_TYPE_COMPOUND);


    while(parser->token->type != TOKEN_TYPE_RBRACE)
        list_push(tree->childs, parser_parse_expr(parser));


    parser_poll(parser, TOKEN_TYPE_RBRACE);

    return tree;
}
