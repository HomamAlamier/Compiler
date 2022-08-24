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
    parser->last_expr = NULL;
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

bool eval_integer_oper(ast_t* oper, int* result) {
    LIST_FOREACH(oper->childs, {
        ast_t* item = node->data;
        if (item->type == AST_TYPE_OPERATION) {
            eval_integer_oper(item, result);
        } else if (item->type == AST_TYPE_INTEGER) {
            *result += *CAST(int*, item->data);
        } else {
            return false;
        }
    })
    return true;
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
        LOG_PUSH_ARGS("%s:(%d:%d): Unexpected token %s [%s] (expected %s)",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 token_type_str(parser->token->type), parser->token->value->buffer, token_type_str(*(int*)data));
        break;
    case PARSER_ERROR_UNHANDLED_EXPER:
        LOG_PUSH_ARGS("%s:(%d:%d): Unhandled experssion %s (value = `%s`)",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 token_type_str(parser->token->type), parser->token->value->buffer);
        break;
    case PARSER_ERROR_ASM_FUNCTION_INVALID_BODY:
        LOG_PUSH_ARGS("%s:(%d:%d): in asm function `%s`: body should only contains strings (unexpected `%s`)",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 data, token_type_str(parser->token->type));
        break;
    case PARSER_ERROR_INDEX_OPERATION_RESULT_NOT_INT:
        LOG_PUSH_ARGS("%s:(%d:%d): Error: cannot index array `%s` with an opertaion that does not result in an integer value",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 data);
        break;
    case PARSER_ERROR_DECL_OPERATION_RESULT_NOT_INT:
        LOG_PUSH_ARGS("%s:(%d:%d): Error: cannot declare array `%s` size with an opertaion that does not result in an integer value",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 data);
        break;
    case PARSER_ERROR_INDEX_OPERATION_RESULT_NEG_INT:
        LOG_PUSH_ARGS("%s:(%d:%d): Error: cannot index array `%s` with a negative integer value",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 data);
        break;
    case PARSER_ERROR_DECL_OPERATION_RESULT_NEG_INT:
        LOG_PUSH_ARGS("%s:(%d:%d): Error: cannot declare array `%s` size with a negative integer value",
                 parser->token->filename->buffer, parser->token->row, parser->token->col - 1,
                 data);
        break;
    }
    exit(1);
}

ast_t* parser_parse_arglist(parser_t* parser, ast_t* parent) {
    parser_poll(parser, TOKEN_TYPE_LPAREN);

    ast_t* tree = init_ast(AST_TYPE_COMPOUND);
    tree->token = parser->token;

    if (parser->token->type != TOKEN_TYPE_RPAREN) {

        list_push(tree->childs, parser_parse_arth(parser, parent));


        /*if (CAST(ast_t*, tree->childs->bottom->data)->type ==  AST_TYPE_COMPOUND) {
            LOG_PUSH_ARGS("found args (name = %s, type = %s)",
                          CAST(ast_t*, tree->childs->bottom->data)->name->buffer,
                          CAST(ast_t*, tree->childs->bottom->data)->data_type->name->buffer);
        } else {
            LOG_PUSH_ARGS("found args (name = %s, type = %s)",
                          CAST(ast_t*, tree->childs->bottom->data)->name->buffer,
                          CAST(ast_t*, tree->childs->bottom->data)->data);
        }*/

        while(parser->token->type == TOKEN_TYPE_COMMA) {
            parser_poll(parser, TOKEN_TYPE_COMMA);
            list_push(tree->childs, parser_parse_arth(parser, parent));
            //LOG_PUSH_ARGS("found args (name = %s, type = %s)",
            //              CAST(ast_t*, tree->childs->bottom->data)->name->buffer,
            //              CAST(ast_t*, tree->childs->bottom->data)->data_type->name->buffer);
        }
    }

    parser_poll(parser, TOKEN_TYPE_RPAREN);

    return tree;
}

ast_t* parser_parse_list(parser_t* parser, ast_t* parent) {

    ast_t* tree = init_ast(AST_TYPE_LIST);
    tree->token = parser->token;
    tree->parent = parent;
    parser_poll(parser, TOKEN_TYPE_LBRAKET);

    if (parser->token->type != TOKEN_TYPE_RBRAKET) {

        list_push(tree->childs, parser_parse_expr(parser, tree));

        while(parser->token->type == TOKEN_TYPE_COMMA) {
            parser_poll(parser, TOKEN_TYPE_COMMA);
            list_push(tree->childs, parser_parse_expr(parser, tree));
        }

    }


    parser_poll(parser, TOKEN_TYPE_RBRAKET);
    return tree;
}

ast_t* parser_parse_template(parser_t* parser) {

    parser_poll(parser, TOKEN_TYPE_LT);

    ast_t* tree = init_ast(AST_TYPE_TEMPLATE);
    tree->token = parser->token;


    list_push(tree->childs, parser_parse_expr(parser, tree));

    while(parser->token->type == TOKEN_TYPE_COMMA) {
        parser_poll(parser, TOKEN_TYPE_COMMA);
        list_push(tree->childs, parser_parse_expr(parser, tree));
    }


    parser_poll(parser, TOKEN_TYPE_GT);

    return tree;
}

ast_t* parser_parse_integer(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_INTEGER);
    tree->token = parser->token;
    tree->data = heap_int(atoi(parser->token->value->buffer));
    parser_poll(parser, TOKEN_TYPE_INTEGER_LITERAL);
    return tree;
}

ast_t* parser_parse_double(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_DOUBLE);
    tree->token = parser->token;
    tree->data = heap_double(atof(parser->token->value->buffer));
    parser_poll(parser, TOKEN_TYPE_DOUBLE_LITERAL);
    return tree;
}

ast_t* parser_parse_string(parser_t* parser) {
    ast_t* tree = init_ast(AST_TYPE_STRING);
    tree->name = init_string("string");
    tree->data = parser->token->value;
    tree->token = parser->token;
    parser_poll(parser, TOKEN_TYPE_STRING_LITERAL);
    return tree;
}

ast_t* parser_parse_call(parser_t* parser) {
    /*ast_t* tree = init_ast(AST_TYPE_CALL);
    tree->name = parser->token->value;
    tree->value = parser_parse_arglist(parser);
    tree->token = parser->token;
    parser_poll(parser, TOKEN_TYPE_STRING_LITERAL);
    return tree;*/
}

ast_t* parser_parse_id(parser_t* parser, ast_t* parent) {
    if (parser->token->type == TOKEN_TYPE_IDENTIFIER) {

        string_t* id = parser->token->value;
        lexer_token_t* token = parser->token;
        parser_poll(parser, TOKEN_TYPE_IDENTIFIER);

        if (string_cmp_str(id, "return")) {
            ast_t* tree = init_ast(AST_TYPE_RETURN);
            tree->token = parser->token;
            tree->name = id;

            tree->value = parser_parse_arth(parser, parent);

            if (parser->token->type == TOKEN_TYPE_SEMI)
                parser_poll(parser, TOKEN_TYPE_SEMI);

            return tree;
        } else if(string_cmp_str(id, "const")) {
            ast_t* tree = init_ast(AST_TYPE_CONSTANT);
            tree->token = parser->token;
            ast_t* var = parser_parse_expr(parser, tree);
            tree->name = var->name;
            tree->data_type = var->data_type;
            tree->parent = parent;
            tree->value = var->value;
            free(var);
            return tree;
        } else if (token_is_operator(parser->token->type) && parser->token->type == TOKEN_TYPE_EQUALS) {
            ast_t* tree = init_ast(AST_TYPE_ASSIGNMENT);
            tree->parent = parent;
            tree->token = parser->token;
            ast_t* lhs = init_ast(AST_TYPE_ACCESS);
            lhs->name = id;
            lhs->token = token;
            list_push(tree->childs, lhs);
            list_push(tree->childs, parser_parse_arth(parser, tree));
            return tree;
        } else if (parser->token->type == TOKEN_TYPE_LPAREN) {
            ast_t* tree = init_ast(AST_TYPE_CALL);
            tree->name = id;
            tree->token = parser->token;
            tree->value = parser_parse_arglist(parser, tree);
            parser_poll(parser, TOKEN_TYPE_SEMI);
            return tree;
        } else {
            LOG_PUSH_ARGS("possible decleration ! (type: %s, name: %s)", id->buffer, parser->token->value->buffer);
            bool is_array = parser->token->type == TOKEN_TYPE_LBRAKET;
            ast_t* tree = init_ast(is_array ? AST_TYPE_VARIABLE_ARRAY : AST_TYPE_VARIABLE);
            tree->parent = parent;
            if (parser->token->type == TOKEN_TYPE_LBRAKET) {
                parser_poll(parser, TOKEN_TYPE_LBRAKET);

                if (parser->token->type != TOKEN_TYPE_RBRAKET) {
                    tree->index = parser_parse_expr(parser, tree);
                    int sz = 0;
                }

                parser_poll(parser, TOKEN_TYPE_RBRAKET);
            }
            bool is_decl = parser->token->type == TOKEN_TYPE_IDENTIFIER;
            if (is_decl) {
                tree->name = parser->token->value;
                tree->data_type = init_complex_data_type(id, is_array, 0);
                tree->token = parser->token;
                parser_poll(parser, TOKEN_TYPE_IDENTIFIER);
            } else {
                tree->name = id;
                tree->token = token;
                tree->type = is_array ?  AST_TYPE_ACCESS_ARRAY : AST_TYPE_ACCESS;
            }

            if (is_array) {
                int sz = 0;
                if (tree->index && tree->index->type  == AST_TYPE_OPERATION) {
                    if (!eval_integer_oper(tree->index, &sz)) {
                        parser_report_and_exit(parser, is_decl ? PARSER_ERROR_DECL_OPERATION_RESULT_NOT_INT : PARSER_ERROR_INDEX_OPERATION_RESULT_NOT_INT, id);
                    } else if (sz < 0) {
                        parser_report_and_exit(parser, is_decl ? PARSER_ERROR_DECL_OPERATION_RESULT_NEG_INT : PARSER_ERROR_INDEX_OPERATION_RESULT_NEG_INT, id);
                    } else {
                        tree->index->data = heap_int(sz);
                    }
                }
            }


            //const char* debug = tree->name->buffer;
            //const char* debug2 = tree->data_type ? tree->data_type->name->buffer : NULL;

            switch (parser->token->type) {
            case TOKEN_TYPE_EQUALS:
            {
                parser_poll(parser, TOKEN_TYPE_EQUALS);
                tree->value = parser_parse_arth(parser, tree);
                parser_poll(parser, TOKEN_TYPE_SEMI);
                break;
            }
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

                bool is_asm = false;

                if (parser->last_expr->type == AST_TYPE_LIST) {
                    LIST_FOREACH(parser->last_expr->childs, {
                        ast_t* item = node->data;
                        if (item->type == AST_TYPE_STRING) {
                            printf("Checking %s..", CAST(string_t*, item->data)->buffer);
                            if (string_cmp_str(item->data, "__asm__")) {
                                tree->type = AST_TYPE_ASM_FUNCTION;
                                is_asm = true;
                            }
                        }
                    })
                }


                ast_t* args = parser_parse_arglist(parser, tree);
                tree->childs = args->childs;
                args->childs = NULL;
                free(args);

                tree->data = init_list();
                list_push(tree->data, parser->lexer->filename);
                list_push(tree->data, heap_size_t(parser->lexer->col));
                list_push(tree->data, heap_size_t(parser->lexer->row));

                switch (parser->token->type) {
                case TOKEN_TYPE_LBRACE:
                    if (is_asm) {
                        parser_poll(parser, TOKEN_TYPE_LBRACE);

                        ast_t* body = init_ast(AST_TYPE_COMPOUND);
                        body->parent = tree;

                        while(parser->token->type != TOKEN_TYPE_RBRACE) {
                            if (parser->token->type == TOKEN_TYPE_STRING_LITERAL) {
                                list_push(body->childs, parser_parse_string(parser));
                            } else {
                                parser_report_and_exit(parser, PARSER_ERROR_ASM_FUNCTION_INVALID_BODY, tree->name->buffer);
                            }
                        }

                        parser_poll(parser, TOKEN_TYPE_RBRACE);

                        tree->value = body;
                    } else {
                        tree->value = parser_parse_expr(parser, tree);
                    }
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
            }

            return tree;
        }

    }
    return NULL;
}

ast_t* parser_parse_expr(parser_t* parser, ast_t* parent) {

    LOG_PUSH_ARGS("Token (type: %s, value: %s)", token_type_str(parser->token->type), parser->token->value->buffer);

    ast_t* tree = NULL;

    switch(parser->token->type) {
    case TOKEN_TYPE_IDENTIFIER: tree = parser_parse_id(parser, parent); break;
    case TOKEN_TYPE_EQUALS: tree = parser_parse_arth(parser, parent); break;
    case TOKEN_TYPE_LBRACE: tree = parser_parse_block(parser); break;
    case TOKEN_TYPE_LBRAKET: tree = parser_parse_list(parser, parent); break;
    case TOKEN_TYPE_INTEGER_LITERAL: tree = parser_parse_integer(parser); break;
    case TOKEN_TYPE_DOUBLE_LITERAL: tree = parser_parse_double(parser); break;
    case TOKEN_TYPE_STRING_LITERAL: tree = parser_parse_string(parser); break;
    default:
    {
        tree = init_ast(AST_TYPE_STATEMENT);
        tree->name = parser->token->value;
        parser_poll(parser, parser->token->type);
    }
    }
    tree->parent = parent;
    parser->last_expr = tree;
    return tree;
}

ast_t* parser_parse_arth(parser_t* parser, ast_t* parent) {

    ast_t* tree = NULL;


    /*switch (parser->token->type) {
    case TOKEN_TYPE_INTEGER_LITERAL:
        tree = parser_parse_integer(parser);
        break;
    case TOKEN_TYPE_DOUBLE_LITERAL:
        tree = parser_parse_double(parser);
        break;
    case TOKEN_TYPE_IDENTIFIER:
    {
        string_t* id = parser->token->value;
        lexer_token_t* token = parser->token;
        parser_poll(parser, TOKEN_TYPE_IDENTIFIER);
        if (parser->token->type == TOKEN_TYPE_LPAREN) {
            tree = init_ast(AST_TYPE_CALL);
            tree->name = id;
            tree->token = token;
            tree->value = parser_parse_arglist(parser);
        } else {
            tree = init_ast(AST_TYPE_ACCESS);
            tree->token = token;
            tree->name = id;
        }
        break;
    }
    default:
        parser_report_and_exit(parser, PARSER_ERROR_UNHANDLED_EXPER, NULL);
    }*/

    const char* debug = parser->token->value->buffer;

    tree = parser_parse_expr(parser, parent);

    if (token_is_operator(parser->token->type)) {
        ast_t* tmp = init_ast(AST_TYPE_OPERATION);
        tmp->token = parser->token;
        tmp->name = parser->token->value;
        list_push(tmp->childs, tree);
        parser_poll(parser, parser->token->type);
        list_push(tmp->childs, parser_parse_arth(parser, parent));
        tree = tmp;
    }

    return tree;
}

ast_t* parser_parse_assign(parser_t* parser) {

    ast_t* tree = init_ast(AST_TYPE_VARIABLE);
    tree->token = parser->token;
    parser_poll(parser, parser->token->type);

    tree->name = parser->token->value;

    if (parser->token->type == TOKEN_TYPE_OPERATOR) {
        ast_t* var = init_ast(AST_TYPE_VARIABLE);
        var->token = parser->token;
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
    tree->token = parser->token;
    parser_poll(parser, TOKEN_TYPE_IDENTIFIER);

    tree->name = init_string("return");
    tree->value = parser_parse_expr(parser, tree);

    parser_poll(parser, TOKEN_TYPE_SEMI);
    return tree;
}

ast_t* parser_parse_block(parser_t* parser) {
    parser_poll(parser, TOKEN_TYPE_LBRACE);

    ast_t* tree = init_ast(AST_TYPE_COMPOUND);
    tree->token = parser->token;

    while(parser->token->type != TOKEN_TYPE_RBRACE)
        list_push(tree->childs, parser_parse_expr(parser, tree));


    parser_poll(parser, TOKEN_TYPE_RBRACE);

    return tree;
}
