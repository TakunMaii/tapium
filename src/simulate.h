#ifndef SIMULATE_H
#define SIMULATE_H
#include "hashmap.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAPE_LENGTH 1024

typedef struct Region Region;
struct Region
{
    long long tape[TAPE_LENGTH];
    int pointer;
};

Region *region_stack[1024];
int region_pointer = 0;

void pushRegionWith(int num)
{
    region_stack[region_pointer++] = (Region*)malloc(sizeof(Region));
    region_stack[region_pointer-1]->pointer = 0;
    memset(region_stack[region_pointer-1]->tape, 0, TAPE_LENGTH);

    if(num > 0)
    {
        if(region_pointer <= 1)
        {
            printf("Trying to push region with params when there's no father region\n");
            exit(1);
        }
        // copy the pointer~pointer+num numbers in father region to the 0~num in this region
        for(int i = region_stack[region_pointer-2]->pointer, counter = 0;
            counter<num;
            i++, counter++)
        {
            region_stack[region_pointer-1]->tape[counter] = region_stack[region_pointer-2]->tape[i];
        }
    }
}

void popRegionWith(int num)
{
    if(num > 0)
    {
        if(region_pointer <= 1)
        {
            printf("Trying to pop region with params when there's no father region\n");
            exit(1);
        }
        // copy the pointer~pointer+num numbers in this region to the pointer~pointer+num in father region
        for(int i = region_stack[region_pointer-2]->pointer,
                j = region_stack[region_pointer-1]->pointer,
                counter = 0;
            counter<num;
            i++, j++, counter++)
        {
            region_stack[region_pointer-1]->tape[j] = region_stack[region_pointer-2]->tape[i];
        }
    }

    free(region_stack[region_pointer-1]);
    region_pointer --;
}

void simulate(Token *token)
{
    int tuple_times_stack[128];
    int tuple_times_pointer = 0;

    while (token) {
        switch (token->kind) {
            case NULLTOK: {
                token = token->next;
            } break;
            case PLUS: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                tape[pointer] += token->num;
                token = token->next;
            } break;
            case MINUS: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                tape[pointer] -= token->num;
                token = token->next;
            } break;
            case RIGHT: {
                region_stack[region_pointer-1]->pointer += token->num;
                token = token->next;
            } break;
            case LEFT: {
                region_stack[region_pointer-1]->pointer -= token->num;
                if(region_stack[region_pointer-1]->pointer<0)
                {
                    printf("Pointer too left!! %d", region_stack[region_pointer-1]->pointer);
                    exit(1);
                }
                token = token->next;
            } break;
            case INPUT: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                if(token->type == INTEGER)
                {
                    scanf("%lld", &tape[pointer]);
                }
                else if(token->type == CHAR)
                {
                    scanf("%c", (char*)&tape[pointer]);
                }
                else if(token->type == STRING)
                {
                    char *str = (char*)malloc(256);
                    scanf("%s", str);
                    tape[pointer] = (long long)str;
                }
                token = token->next;
            } break;
            case OUTPUT: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                if(token->type == INTEGER)
                {
                    printf("%lld", tape[pointer]);
                }
                else if(token->type == CHAR)
                {
                    printf("%c", (int)tape[pointer]);
                }
                else if(token->type == STRING)
                {
                    printf("%s", (char*)tape[pointer]);
                }
                token = token->next;
            } break;
            case START_TUPLE: {
                // store tuple time num
                tuple_times_stack[tuple_times_pointer++] = token->pair->num;
                if(token->pair->num <= 0)// not run the tuple
                {
                    tuple_times_pointer--;
                    token = token->pair;
                }
                token = token->next;
            } break;
            case END_TUPLE: {
                if(--tuple_times_stack[tuple_times_pointer - 1])
                {
                    // jump to end start of the tuple
                    token = token->pair->next;
                }
                else
                {
                    // tuple run finished 
                    token = token->next;
                    tuple_times_pointer--;
                }
            } break;
            case WHEN: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                if(!tape[pointer])
                {
                    token = token->pair->next;
                }
                else token = token->next;
            } break;
            case END: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                if(tape[pointer])
                {
                    token = token->pair->next;
                }
                else token = token->next;
            } break;
            case MACRO: {
                Token *macro_content = token_hashmap_get(macroMap, token->macro_name);
                simulate(macro_content);
                token = token->next;
            } break;
            case EMBED: {
                token->embedded_func();
                token = token->next;
            } break;
            case NEW_STRING: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                tape[pointer] = (long long)token->ptr;
                token = token->next;
            } break;
            case START_REGION: {
                pushRegionWith(token->num);
                token = token->next;
            } break;
            case END_REGION: {
                popRegionWith(token->num);
                token = token->next;
            } break;
        }
    }
}

#endif
