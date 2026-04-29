#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "simulate.h"
#include "token.h"
#include "file.h"
#include "embeded_func.h"
#include "compile.h"
#include "ir.h"

int main(int argn, char** argv)
{
    if(argn < 2)
    {
        printf("Usage:\n");
        printf("  %s <filename>\n", argv[0]);
        printf("  %s c <filename> [-o output.c]\n", argv[0]);
        printf("  %s build <filename> [-o output.exe]\n", argv[0]);
        printf("  %s ir <filename> [-o output.ir]\n", argv[0]);
        exit(1);
    }

    if(strcmp(argv[1], "c") == 0)
    {
        if(argn < 3)
        {
            printf("Usage: %s c <filename> [-o output.c]\n", argv[0]);
            return 1;
        }

        const char *input_path = argv[2];
        char *output_path_owned = 0;
        const char *output_path = 0;

        if(argn >= 5 && strcmp(argv[3], "-o") == 0)
        {
            output_path = argv[4];
        }
        else if(argn == 3)
        {
            output_path_owned = default_c_output_name(input_path);
            if(!output_path_owned)
            {
                return 1;
            }
            output_path = output_path_owned;
        }
        else
        {
            printf("Usage: %s c <filename> [-o output.c]\n", argv[0]);
            return 1;
        }

        int c_code = compile_to_c_source(input_path, output_path);
        if(c_code == 0)
        {
            printf("C generated: %s\n", output_path);
        }
        free(output_path_owned);
        return c_code;
    }

    if(strcmp(argv[1], "ir") == 0)
    {
        if(argn < 3)
        {
            printf("Usage: %s ir <filename> [-o output.ir]\n", argv[0]);
            return 1;
        }

        const char *input_path = argv[2];
        char *output_path_owned = 0;
        const char *output_path = 0;
        if(argn >= 5 && strcmp(argv[3], "-o") == 0)
        {
            output_path = argv[4];
        }
        else if(argn == 3)
        {
            output_path_owned = default_ir_output_name(input_path);
            if(!output_path_owned)
            {
                return 1;
            }
            output_path = output_path_owned;
        }
        else
        {
            printf("Usage: %s ir <filename> [-o output.ir]\n", argv[0]);
            return 1;
        }

        long fileLength;
        char *fileContent = read_file(input_path, &fileLength);
        if(!fileContent)
        {
            free(output_path_owned);
            return 1;
        }

        macroMap = token_hashmap_create(127);
        register_embeded_func();
        int length = 0;
        Token *tokens = tokenize(fileContent, &length, input_path);
        int ir_code = write_ir_file(output_path, tokens);
        if(ir_code == 0)
        {
            printf("IR generated: %s\n", output_path);
        }
        free(output_path_owned);
        return ir_code;
    }

    if(strcmp(argv[1], "build") == 0)
    {
        if(argn < 3)
        {
            printf("Usage: %s build <filename> [-o output.exe]\n", argv[0]);
            return 1;
        }

        const char *input_path = argv[2];
        char *output_path_owned = 0;
        const char *output_path = 0;

        if(argn >= 5 && strcmp(argv[3], "-o") == 0)
        {
            output_path = argv[4];
        }
        else if(argn == 3)
        {
            output_path_owned = default_output_name(input_path);
            if(!output_path_owned)
            {
                return 1;
            }
            output_path = output_path_owned;
        }
        else
        {
            printf("Usage: %s build <filename> [-o output.exe]\n", argv[0]);
            return 1;
        }

        int build_code = compile_to_executable(input_path, output_path);
        if(build_code == 0)
        {
            printf("Build succeeded: %s\n", output_path);
        }
        free(output_path_owned);
        return build_code;
    }

    // read the program file
    long fileLength;
    char *fileContent = read_file(argv[1], &fileLength);
    if(!fileContent)
    {
        return 1;
    }

    // initialize macroMap
    macroMap = token_hashmap_create(127);

    // tokenize the program
    int length = 0;
    Token *tokens = tokenize(fileContent, &length, argv[1]);
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
