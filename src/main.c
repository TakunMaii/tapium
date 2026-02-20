#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"
#include "simulate.h"
#include "token.h"
#include "file.h"

void tap_get_rand()
{
    tape[pointer] = rand();
}

void register_embeded_func()
{
    Token *token = newToken(EMBED);
    token->embedded_func = tap_get_rand;
    token_hashmap_put(macroMap, "rand", token);
}

int main(int argn, char** argv)
{
    if(argn < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    // initialize macroMap
    macroMap = token_hashmap_create(127);

    long fileLength;
    char *fileContent = read_file(argv[1], &fileLength);

    int length = 0;
    Token *tokens = tokenize(fileContent, &length);
    printf("===TOKENS===\n");
    printTokens(tokens);
    printf("===END TOKENS===\n");

    memset(tape, 0, TAPE_LENGTH);
    register_embeded_func();
    simulate(tokens);
    return 0;
}
