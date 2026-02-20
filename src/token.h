#include <stdio.h>
#include <stdlib.h>

typedef enum TokenType TokenType;
enum TokenType
{
    NULLTOK,
    PLUS,
    MINUS,
    RIGHT,
    LEFT,
    INPUT,
    OUTPUT,
    START,
    END,
};

typedef struct Token Token;
struct Token
{
    Token *next;
    TokenType kind;
    int num;
    Token *pair;
};

Token *newToken(TokenType kind)
{
    Token *token = (Token*)malloc(sizeof(Token));
    token->next = 0;
    token->pair = 0;
    token->kind = kind;
    return token;
}

void printTokens(Token *token)
{
    while (token) {
        switch(token->kind)
        {
        case NULLTOK: printf("token: NULLTOK\n"); break;
        case PLUS: printf("token: PLUS\n"); break;
        case MINUS: printf("token: MINUS\n"); break;
        case RIGHT: printf("token: RIGHT\n"); break;
        case LEFT: printf("token: LEFT\n"); break;
        case INPUT: printf("token: INPUT\n"); break;
        case OUTPUT: printf("token: OUTPUT\n"); break;
        case START: printf("token: START\n"); break;
        case END: printf("token: END\n"); break;
        }
        token = token->next;
    }
}

Token* tokenize(char *program)
{
    Token *pair_stack[1024];
    int pair_pointer = 0;

    Token *token = newToken(NULLTOK), *head = token;
    while (*program) {
        switch (*program) {
            case ' ':
            case '\n': {
                program++;
            } break;
            case '#': {//comment
                while (*program!='\n') {
                    program++;
                }
                program++;
            } break;
            case '+': {
                token->next = newToken(PLUS);
                token = token->next;
                program++;
            } break;
            case '-': {
                token->next = newToken(MINUS);
                token = token->next;
                program++;
            } break;
            case ',': {
                token->next = newToken(INPUT);
                token = token->next;
                program++;
            } break;
            case '.': {
                token->next = newToken(OUTPUT);
                token = token->next;
                program++;
            } break;
            case '[': {
                token->next = newToken(START);
                token = token->next;
                pair_stack[pair_pointer++] = token;
                program++;
            } break;
            case ']': {
                token->next = newToken(END);
                token = token->next;
                if(pair_pointer == 0)
                {
                    printf("Error ]\n");
                    exit(1);
                }
                token->pair = pair_stack[--pair_pointer];
                token->pair->pair = token;
                program++;
            } break;
            case '>': {
                token->next = newToken(RIGHT);
                token = token->next;
                program++;
            } break;
            case '<': {
                token->next = newToken(LEFT);
                token = token->next;
                program++;
            } break;
            default:
                printf("Unknown char %c\n", *program);
                exit(1);
        }
    }
    return head;
}

