#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"
#include "simulate.h"
#include "token.h"
#include "file.h"
#include "embeded_func.h"

int main(int argn, char** argv)
{
    if(argn < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    // read the program file
    long fileLength;
    char *fileContent = read_file(argv[1], &fileLength);

    // initialize macroMap
    macroMap = token_hashmap_create(127);

    // tokenize the program
    int length = 0;
    Token *tokens = tokenize(fileContent, &length);
#ifdef DEBUG
    printf("===TOKENS===\n");
    printTokens(tokens);
    printf("===END TOKENS===\n");
#endif

    pushRegionWith(0);// initial region
    register_embeded_func();

    // simulate
    simulate(tokens);

    return 0;
}
