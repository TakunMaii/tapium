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

    long long stack[TAPE_LENGTH];
    int stack_pointer;
};

Region *region_stack[1024];
int region_pointer = 0;
Token *runtime_current_token = NULL;

#define REGION (region_stack[region_pointer-1])
#define TAPE (REGION->tape)
#define POINTER (REGION->pointer)
#define STACK (REGION->stack)
#define STPTR (REGION->stack_pointer)

void runtime_error(const char *message)
{
    if(runtime_current_token)
    {
        printf(
            "Runtime error at %s:%d:%d: %s\n",
            runtime_current_token->source[0] ? runtime_current_token->source : "<unknown>",
            runtime_current_token->line,
            runtime_current_token->column,
            message
        );
    }
    else
    {
        printf("Runtime error: %s\n", message);
    }
    exit(1);
}

void ensure_pointer_in_bounds(int pointer)
{
    if(pointer < 0 || pointer >= TAPE_LENGTH)
    {
        runtime_error("pointer out of tape bounds");
    }
}

void ensure_stack_not_empty()
{
    if(STPTR <= 0)
    {
        runtime_error("stack underflow");
    }
}

void ensure_stack_not_full()
{
    if(STPTR >= TAPE_LENGTH)
    {
        runtime_error("stack overflow");
    }
}

int resolve_num(Token *token)
{
    if(token->num_using_stack_top)
    {
        ensure_stack_not_empty();
        return STACK[STPTR-1];
    }
    return token->num;
}

void pushRegionWith(int num)
{
    if(region_pointer >= 1024)
    {
        runtime_error("region stack overflow");
    }
    if(num < 0)
    {
        runtime_error("region copy length can not be negative");
    }
    region_stack[region_pointer++] = (Region*)malloc(sizeof(Region));
    region_stack[region_pointer-1]->pointer = 0;
    memset(region_stack[region_pointer-1]->tape, 0, sizeof(region_stack[region_pointer-1]->tape));

    region_stack[region_pointer-1]->stack_pointer = 0;
    memset(region_stack[region_pointer-1]->stack, 0, sizeof(region_stack[region_pointer-1]->stack));

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
            ensure_pointer_in_bounds(i);
            ensure_pointer_in_bounds(counter);
            region_stack[region_pointer-1]->tape[counter] = region_stack[region_pointer-2]->tape[i];
        }
    }
}

void popRegionWith(int num)
{
    if(region_pointer <= 0)
    {
        runtime_error("region stack underflow");
    }
    if(num < 0)
    {
        runtime_error("region copy length can not be negative");
    }
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
            ensure_pointer_in_bounds(i);
            ensure_pointer_in_bounds(j);
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
        runtime_current_token = token;
        switch (token->kind) {
            case NULLTOK: {
                token = token->next;
            } break;
            case PLUS: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                ensure_pointer_in_bounds(pointer);
                int num = resolve_num(token);
                tape[pointer] += num;
                token = token->next;
            } break;
            case MINUS: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                ensure_pointer_in_bounds(pointer);
                int num = resolve_num(token);
                tape[pointer] -= num;
                token = token->next;
            } break;
            case RIGHT: {
                int num = resolve_num(token);
                region_stack[region_pointer-1]->pointer += num;
                ensure_pointer_in_bounds(region_stack[region_pointer-1]->pointer);
                token = token->next;
            } break;
            case LEFT: {
                int num = resolve_num(token);
                region_stack[region_pointer-1]->pointer -= num;
                ensure_pointer_in_bounds(region_stack[region_pointer-1]->pointer);
                token = token->next;
            } break;
            case INPUT: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                ensure_pointer_in_bounds(pointer);
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
                ensure_pointer_in_bounds(pointer);
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
                if(tuple_times_pointer >= 128)
                {
                    runtime_error("tuple nesting too deep");
                }
                tuple_times_stack[tuple_times_pointer++] = resolve_num(token->pair);
                if(tuple_times_stack[tuple_times_pointer-1] <= 0)// not run the tuple
                {
                    tuple_times_pointer--;
                    token = token->pair;
                }
                token = token->next;
            } break;
            case END_TUPLE: {
                if(tuple_times_pointer <= 0)
                {
                    runtime_error("tuple stack underflow");
                }
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
                ensure_pointer_in_bounds(pointer);
                if(!tape[pointer])
                {
                    token = token->pair->next;
                }
                else token = token->next;
            } break;
            case END: {
                long long *tape = region_stack[region_pointer-1]->tape;
                int pointer = region_stack[region_pointer-1]->pointer;
                ensure_pointer_in_bounds(pointer);
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
                ensure_pointer_in_bounds(pointer);
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
