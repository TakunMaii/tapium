#include <stdio.h>
#include <stdbool.h>
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
    token->num = 1;
    token->kind = kind;
    return token;
}

void printTokens(Token *token)
{
    while (token) {
        switch(token->kind)
        {
        case NULLTOK: printf("token: NULLTOK\n"); break;
        case PLUS: printf("token: PLUS %d\n", token->num); break;
        case MINUS: printf("token: MINUS %d\n", token->num); break;
        case RIGHT: printf("token: RIGHT %d\n", token->num); break;
        case LEFT: printf("token: LEFT %d\n", token->num); break;
        case INPUT: printf("token: INPUT\n"); break;
        case OUTPUT: printf("token: OUTPUT\n"); break;
        case START: printf("token: START\n"); break;
        case END: printf("token: END\n"); break;
        }
        token = token->next;
    }
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

int seek_num(char *program, Token *token)
{
    int num = 0;
    int counter = 0;
    while (is_digit(*program)) {
        num = num * 10 + (*program - '0');
        program++;
        counter++;
    }
    if(counter != 0)
    {
        token->num = num;
    }
    return counter;
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
                program += seek_num(program, token);
            } break;
            case '-': {
                token->next = newToken(MINUS);
                token = token->next;
                program++;
                program += seek_num(program, token);
            } break;
            case '>': {
                token->next = newToken(RIGHT);
                token = token->next;
                program++;
                program += seek_num(program, token);
            } break;
            case '<': {
                token->next = newToken(LEFT);
                token = token->next;
                program++;
                program += seek_num(program, token);
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
            default:
                printf("Unknown char %c\n", *program);
                exit(1);
        }
    }
    return head;
}

