#include "token.h"
#include <stdio.h>
#include <string.h>

#define TAPE_LENGTH 1024
int tape[TAPE_LENGTH];
int pointer = 0;

void simulate(Token *token)
{
    memset(tape, 0, TAPE_LENGTH);

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
            case START: {
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
            default: {
                printf("Unkown token type: %d", token->kind);
                exit(1);
            }
        }
    }
}

