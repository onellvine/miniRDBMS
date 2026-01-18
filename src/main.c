#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "executor.h"
#include "lexer.h"
#include "myparser.h"
#include "web.h"


void execute_sql(const char *sql)
{
    Lexer l;
    Parser p;

    lexer_init(&l, sql);
    parser_init(&p, &l);

    ASTNode *node = parse_statement(&p);
    if(!node) return;

    execute(node);
    ast_free(node);
}



int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: %s [repl [, http]]", argv[0]);
        return EXIT_FAILURE;
    }

    char *mode = argv[1];

    if(strncmp(mode, "repl", 4) == 0)
    {
        char input[1024];

        printf("miniRDBMS ready. Type SQL or 'exit'.\n");

        while(1)
        {
            printf("rdbms> ");
            if(!fgets(input, sizeof(input), stdin))
                break;

            if(strncmp(input, "exit", 4) == 0)
                break;

            execute_sql(input);
        }     
    }
    else if(strncmp(mode, "http", 4) == 0)
    {
        mkdir("data", 0755);
        start_http_server();
        return 0;
    }
    else
    {
        fprintf(stderr, "Error: unknown mode '%s'\n", mode);
        return EXIT_FAILURE;
    }


    return 0;
}
