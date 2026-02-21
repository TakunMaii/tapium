#ifndef TOKEN_H
#define TOKEN_H
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "hashmap.h"
#include "file.h"

TokenHashMap *macroMap;

typedef enum TokenType TokenType;
enum TokenType
{
    EMBED,
    NULLTOK,
    PLUS,
    MINUS,
    RIGHT,
    LEFT,
    INPUT,
    OUTPUT,
    WHEN,//{
    END,//}
    START_TUPLE,
    END_TUPLE,
    NEW_STRING,
    MACRO,
};

typedef enum Type Type;
enum Type
{
    INTEGER,
    CHAR,
    STRING,
};

typedef struct Token Token;
struct Token
{
    Token *next;
    TokenType kind;
    Token *pair;
    int num;

    Type type;

    char macro_name[64];

    void (*embedded_func)();

    void *ptr;
};

Token *newToken(TokenType kind)
{
    Token *token = (Token*)malloc(sizeof(Token));
    token->next = 0;
    token->pair = 0;
    token->num = 1;
    token->kind = kind;
    token->type = CHAR;
    return token;
}

void printTokens(Token *token)
{
    while (token) {
        switch(token->kind)
        {
        case EMBED: printf("token: EMBED\n"); break;
        case NULLTOK: printf("token: NULLTOK\n"); break;
        case PLUS: printf("token: PLUS %d\n", token->num); break;
        case MINUS: printf("token: MINUS %d\n", token->num); break;
        case RIGHT: printf("token: RIGHT %d\n", token->num); break;
        case LEFT: printf("token: LEFT %d\n", token->num); break;
        case INPUT: printf("token: INPUT\n"); break;
        case OUTPUT: printf("token: OUTPUT\n"); break;
        case WHEN: printf("token: START\n"); break;
        case END: printf("token: END\n"); break;
        case START_TUPLE: printf("token: START_TUPLE\n"); break;
        case END_TUPLE: printf("token: END_TUPLE %d\n", token->num); break;
        case MACRO: printf("token: MACRO %s\n", token->macro_name); break;
        case NEW_STRING: printf("token: NEW_STRING %s\n", (char*)token->ptr); break;
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
    if (*program == '\'')
    { // single char
        program++;
        counter++;// jump '
        token->num = *program;
        program++;
        counter++;
        if(*program != '\'')
        {
            printf("Wrong char\n");
            exit(1);
        }
        program++;
        counter++;// jump '
    }
    else 
    { // digitals
        while (is_digit(*program)) {
            num = num * 10 + (*program - '0');
            program++;
            counter++;
        }
        if(counter != 0)
        {
            token->num = num;
        }
    }
    return counter;
}

bool is_identifier(char c)
{
    return (c>='a' && c<='z') ||
           (c>='A' && c<='Z') ||
           (c == '_');
}

int seek_identifier(char *program, char *identifer)
{
    int pointer = 0;
    int counter = 0;
    while (is_identifier(*program)) {
        identifer[pointer++] = *program;
        program++;
        counter++;
        if(counter > 64)
        {
            printf("identifer too long(> 64)\n");
            exit(1);
        }
    }
    identifer[pointer++] = '\0';
    return counter;
}

Token* tokenize(char *program, int *consumed_length)
{
    char *program_head = program;

    Token *pair_stack[1024];
    int pair_pointer = 0;
    Token *tuple_stack[1024];
    int tuple_pointer = 0;

    Token *token = newToken(NULLTOK), *head = token;
    while (*program) {
        if(is_identifier(*program))
        {
            // call macro
            token->next = newToken(MACRO);
            token = token->next;
            program += seek_identifier(program, token->macro_name);
            continue;
        }

        switch (*program) {
            case ';': {
                program++;
                *consumed_length = program - program_head;
                return head;
            } break;
            case '%': {// include
                program++;
                char include_path[128];
                int include_length = 0;
                while(*program != '\n')
                {
                    include_path[include_length++] = *(program++);
                }
                include_path[include_length] = '\0';
                long include_program_length;
                int include_token_length;
                char *include_program = read_file(include_path, &include_program_length);
                Token *include_token = tokenize(include_program, &include_token_length);
                Token *include_tail = include_token;
                while (include_tail->next) {
                    include_tail = include_tail->next;
                }
                include_tail->next = head;
                head = include_token;
            } break;
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
                if(*program == '%')
                {
                    token->type = STRING;
                    program++;
                }
                else if(*program == '#')
                {
                    token->type = INTEGER;
                    program++;
                }
            } break;
            case '.': {
                token->next = newToken(OUTPUT);
                token = token->next;
                program++;
                if(*program == '%')
                {
                    token->type = STRING;
                    program++;
                }
                else if(*program == '#')
                {
                    token->type = INTEGER;
                    program++;
                }
            } break;
            case '(': {
                token->next = newToken(START_TUPLE);
                token = token->next;
                tuple_stack[tuple_pointer++] = token;
                program++;
            } break;
            case ')': {
                token->next = newToken(END_TUPLE);
                token = token->next;
                if(tuple_pointer== 0)
                {
                    printf("Error )\n");
                    exit(1);
                }
                token->pair = tuple_stack[--tuple_pointer];
                token->pair->pair = token;
                program++;
                program+=seek_num(program, token);
            } break;
            case '{': {
                token->next = newToken(WHEN);
                token = token->next;
                pair_stack[pair_pointer++] = token;
                program++;
            } break;
            case '}': {
                token->next = newToken(END);
                token = token->next;
                if(pair_pointer == 0)
                {
                    printf("Error }\n");
                    exit(1);
                }
                token->pair = pair_stack[--pair_pointer];
                token->pair->pair = token;
                program++;
            } break;
            case '@': {// define macro
                program++;//jump @

                // seek macro name
                char macro_name[64];
                program += seek_identifier(program, macro_name);

                // seek macro content
                int length = 0;
                Token *macro_content = tokenize(program, &length);
                program += length;

                printf("Define Macro [%s]:\n", macro_name);
                printTokens(macro_content);

                token_hashmap_put(macroMap, macro_name, macro_content);
            } break;
            case '\"': {
                program++;
                char *str = (char*)malloc(256);
                int length = 0;
                while(*program != '\"')
                {
                    str[length++] = *(program++);
                    if(length>256)
                    {
                        printf("string too long!!\n");
                        exit(1);
                    }
                }
                program++;//jump the second "
                str[length++] = '\0';
                token->next = newToken(NEW_STRING);
                token->next->ptr = str;
                token = token->next;
            } break;
            default:
                printf("Unknown char %c\n", *program);
                exit(1);
        }
    }
    *consumed_length = program - program_head;
    return head;
}

#endif
