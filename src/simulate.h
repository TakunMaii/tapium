#ifndef SIMULATE_H
#define SIMULATE_H
#include "hashmap.h"
#include "token.h"
#include <stdio.h>
#include <string.h>

#define TAPE_LENGTH 1024
int tape[TAPE_LENGTH];
int pointer = 0;

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
                tape[pointer] += token->num;
                token = token->next;
            } break;
            case MINUS: {
                tape[pointer] -= token->num;
                token = token->next;
            } break;
            case RIGHT: {
                pointer += token->num;
                token = token->next;
            } break;
            case LEFT: {
                pointer -= token->num;
                if(pointer<0)
                {
                    printf("Pointer too left!! %d", pointer);
                    exit(1);
                }
                token = token->next;
            } break;
            case INPUT: {
                scanf("%d", &tape[pointer]);
                token = token->next;
            } break;
            case OUTPUT: {
                printf("%c", tape[pointer]);
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
                if(!tape[pointer])
                {
                    token = token->pair->next;
                }
                else token = token->next;
            } break;
            case END: {
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
        }
    }
}

#endif
