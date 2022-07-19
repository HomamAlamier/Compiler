#include <stdio.h>
#include <include/lexer.h>
#include <include/io.h>
#include <include/parser.h>
#include <include/strings.h>
#include <include/assembler.h>
#include <include/types.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    const char* filename = "examples/main.lang";

    char* src = read_file_full(filename);

    lexer_t* lexer = init_lexer(filename, init_string(src));

    lexer_token_t* token = NULL;

    parser_t* parser = init_parser(lexer);

    ast_t* root = init_ast(AST_TYPE_COMPOUND);
    root->name = init_string("root");
    while(parser->token->type != TOKEN_TYPE_EOF)
        list_push(root->childs, parser_parse_expr(parser));




    dump_ast(root);

    analyzer_t* ana = init_analyzer();
    analyzer_analyze(ana, root);

    if (ana->errors->size > 0) {
        LIST_FOREACH(ana->errors, {
            printf("%s\n", CAST(string_t*, node->data)->buffer);
        });
        exit(1);
    }

    assembler_t* as = init_assembler(ana);
    assembler_generate(as, root, NULL);


    if (as->output) {
        list_node_t* node = as->output->top;
        while(node) {
            printf("%s\n", CAST(string_t*, node->data)->buffer);
            node = node->next;
        }
    }


    /*while ((token = lexer_get_token(lexer))->type != TOKEN_TYPE_EOF) {
        dump_lexer_token(token);
    }*/
}
