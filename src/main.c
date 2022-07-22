#include <stdio.h>
#include <include/lexer.h>
#include <include/io.h>
#include <include/parser.h>
#include <include/strings.h>
#include <include/assembler.h>
#include <include/types.h>
#include <include/preprocessor.h>
#include <assert.h>

string_t* int_to_hex(int x) {
    char buf[5];
    sprintf(buf, "%x", x);
    return string_format("0x%s", buf);
}


int main(int argc, char *argv[])
{

    const char* debug = int_to_hex(255)->buffer;

    const char* filename = "examples/main.lang";

    char* src = read_file_full(filename);

    lexer_t* lexer = init_lexer(filename, init_string(src));

    preprocessor_t* pre = init_preprocessor(lexer->filename, lexer->src);
    preprocesser_preprocess(pre);

    lexer_token_t* token = NULL;

    parser_t* parser = init_parser(lexer);

    ast_t* root = init_ast(AST_TYPE_COMPOUND);
    root->name = init_string("root");
    while(parser->token->type != TOKEN_TYPE_EOF)
        list_push(root->childs, parser_parse_expr(parser, root));




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
    assembler_generate_data(as);


    if (as->output) {
        list_node_t* node = as->output->text->top;
        while(node) {
            printf("%s\n", CAST(string_t*, node->data)->buffer);
            node = node->next;
        }
    }

    if (as->output) {
        list_node_t* node = as->output->data->top;
        while(node) {
            printf("%s\n", CAST(string_t*, node->data)->buffer);
            node = node->next;
        }
    }


    dump_assembler(as);

    if (!assembler_write_output(as, "out.asm")) {
        printf("Cannot write output !");
        exit(1);
    }

    /*while ((token = lexer_get_token(lexer))->type != TOKEN_TYPE_EOF) {
        dump_lexer_token(token);
    }*/
    return 0;
}
