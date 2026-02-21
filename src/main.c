#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"
#include "simulate.h"
#include "token.h"
#include "file.h"

void tap_get_rand()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    tape[pointer] = rand();
}

void tap_add()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    tape[pointer] = tape[pointer] + tape[pointer+1];
}

void tap_sub()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    tape[pointer] = tape[pointer] - tape[pointer+1];
}

void tap_mul()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    tape[pointer] = tape[pointer] * tape[pointer+1];
}

void tap_div()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    tape[pointer] = tape[pointer] / tape[pointer+1];
}

void register_embeded_func()
{
    Token *token = newToken(EMBED);
    token->embedded_func = tap_get_rand;
    token_hashmap_put(macroMap, "rand", token);

    token = newToken(EMBED);
    token->embedded_func = tap_add;
    token_hashmap_put(macroMap, "add", token);

    token = newToken(EMBED);
    token->embedded_func = tap_sub;
    token_hashmap_put(macroMap, "sub", token);

    token = newToken(EMBED);
    token->embedded_func = tap_mul;
    token_hashmap_put(macroMap, "mul", token);

    token = newToken(EMBED);
    token->embedded_func = tap_div;
    token_hashmap_put(macroMap, "div", token);
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

    pushRegionWith(0);// initial region
    register_embeded_func();
    simulate(tokens);
    return 0;
}
