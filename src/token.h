#ifndef TOKEN_H
#define TOKEN_H
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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
    WHEN,//[
    END,//]
    START_TUPLE,
    END_TUPLE,
    START_REGION,//{
    END_REGION,//}
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
#define TOKEN_SOURCE_MAX 260
struct Token
{
    Token *next;
    TokenType kind;
    Token *pair;

    int num;
    bool num_using_stack_top;

    Type type;

    char macro_name[64];
    char source[TOKEN_SOURCE_MAX];
    int line;
    int column;

    void (*embedded_func)();

    void *ptr;
};

Token *newToken(TokenType kind)
{
    Token *token = (Token*)malloc(sizeof(Token));
    token->next = 0;
    token->pair = 0;
    token->num = 1;
    token->num_using_stack_top = false;
    token->kind = kind;
    token->type = CHAR;
    token->source[0] = '\0';
    token->line = 0;
    token->column = 0;
    return token;
}

Token *newTokenAt(TokenType kind, const char *source, int line, int column)
{
    Token *token = newToken(kind);
    if(source)
    {
        snprintf(token->source, TOKEN_SOURCE_MAX, "%s", source);
    }
    token->line = line;
    token->column = column;
    return token;
}

void compute_line_col(
    const char *program_head,
    const char *program_cur,
    int start_line,
    int start_col,
    int *line,
    int *col
)
{
    *line = start_line;
    *col = start_col;
    for(const char *p = program_head; p < program_cur; p++)
    {
        if(*p == '\n')
        {
            (*line)++;
            *col = 1;
        }
        else
        {
            (*col)++;
        }
    }
}

void parse_error(
    const char *source,
    const char *program_head,
    const char *program_cur,
    int start_line,
    int start_col,
    const char *message
)
{
    int line, col;
    compute_line_col(program_head, program_cur, start_line, start_col, &line, &col);
    printf("Parse error at %s:%d:%d: %s\n", source ? source : "<unknown>", line, col, message);
    exit(1);
}

void parse_error_token(Token *token, const char *message)
{
    printf(
        "Parse error at %s:%d:%d: %s\n",
        token->source[0] ? token->source : "<unknown>",
        token->line,
        token->column,
        message
    );
    exit(1);
}

Token *newTokenAtCursor(
    TokenType kind,
    const char *source,
    const char *program_head,
    const char *program_cur,
    int start_line,
    int start_col
)
{
    int line, col;
    compute_line_col(program_head, program_cur, start_line, start_col, &line, &col);
    return newTokenAt(kind, source, line, col);
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
        case START_REGION: printf("token: START_REGION\n"); break;
        case END_REGION: printf("token: END_REGION %d\n", token->num); break;
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
            return -1;
        }
        program++;
        counter++;// jump '
    }
    else if(*program == '$')
    { // stack top
        program++;
        counter++;
        token->num_using_stack_top = true;
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
            return -1;
        }
    }
    identifer[pointer++] = '\0';
    return counter;
}

Token* tokenize_internal(
    char *program,
    int *consumed_length,
    const char *source,
    int start_line,
    int start_col
)
{
    char *program_head = program;

    Token *pair_stack[1024];
    int pair_pointer = 0;
    Token *tuple_stack[1024];
    int tuple_pointer = 0;

    Token *token = newTokenAt(NULLTOK, source, start_line, start_col), *head = token;
    while (*program) {
        if(is_identifier(*program))
        {
            // call macro
            token->next = newTokenAtCursor(MACRO, source, program_head, program, start_line, start_col);
            token = token->next;
            int len = seek_identifier(program, token->macro_name);
            if(len < 0)
            {
                parse_error(source, program_head, program, start_line, start_col, "identifier too long (>64)");
            }
            program += len;
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
                while(*program != '\n' && *program != '\0')
                {
                    if(include_length >= 127)
                    {
                        parse_error(source, program_head, program, start_line, start_col, "include path too long (>127)");
                    }
                    include_path[include_length++] = *(program++);
                }
                while(include_length > 0 && (include_path[include_length-1] == '\r' || include_path[include_length-1] == ' '))
                {
                    include_length--;
                }
                include_path[include_length] = '\0';
                long include_program_length;
                int include_token_length;
                char *include_program = read_file(include_path, &include_program_length);
                if(!include_program)
                {
                    parse_error(source, program_head, program, start_line, start_col, "failed to read include file");
                }
                Token *include_token = tokenize_internal(include_program, &include_token_length, include_path, 1, 1);
                Token *include_tail = include_token;
                while (include_tail->next) {
                    include_tail = include_tail->next;
                }
                include_tail->next = head;
                head = include_token;
                free(include_program);
            } break;
            case ' ':
            case '\t':
            case '\r':
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
                token->next = newTokenAtCursor(PLUS, source, program_head, program, start_line, start_col);
                token = token->next;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program += len;
            } break;
            case '-': {
                token->next = newTokenAtCursor(MINUS, source, program_head, program, start_line, start_col);
                token = token->next;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program += len;
            } break;
            case '>': {
                token->next = newTokenAtCursor(RIGHT, source, program_head, program, start_line, start_col);
                token = token->next;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program += len;
           } break;
            case '<': {
                token->next = newTokenAtCursor(LEFT, source, program_head, program, start_line, start_col);
                token = token->next;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program += len;
            } break;
            case ',': {
                token->next = newTokenAtCursor(INPUT, source, program_head, program, start_line, start_col);
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
                token->next = newTokenAtCursor(OUTPUT, source, program_head, program, start_line, start_col);
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
                token->next = newTokenAtCursor(START_TUPLE, source, program_head, program, start_line, start_col);
                token = token->next;
                tuple_stack[tuple_pointer++] = token;
                program++;
            } break;
            case ')': {
                token->next = newTokenAtCursor(END_TUPLE, source, program_head, program, start_line, start_col);
                token = token->next;
                if(tuple_pointer== 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "unexpected ')'");
                }
                token->pair = tuple_stack[--tuple_pointer];
                token->pair->pair = token;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program+=len;
            } break;
            case '[': {
                token->next = newTokenAtCursor(WHEN, source, program_head, program, start_line, start_col);
                token = token->next;
                pair_stack[pair_pointer++] = token;
                program++;
            } break;
            case ']': {
                token->next = newTokenAtCursor(END, source, program_head, program, start_line, start_col);
                token = token->next;
                if(pair_pointer == 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "unexpected ']'");
                }
                token->pair = pair_stack[--pair_pointer];
                token->pair->pair = token;
                program++;
            } break;
            case '{': {
                token->next = newTokenAtCursor(START_REGION, source, program_head, program, start_line, start_col);
                token = token->next;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program += len;
            } break;
            case '}': {
                token->next = newTokenAtCursor(END_REGION, source, program_head, program, start_line, start_col);
                token = token->next;
                program++;
                int len = seek_num(program, token);
                if(len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "invalid char literal");
                }
                program += len;
            } break;
            case '@': {// define macro
                program++;//jump @

                // seek macro name
                char macro_name[64];
                int macro_name_len = seek_identifier(program, macro_name);
                if(macro_name_len < 0)
                {
                    parse_error(source, program_head, program, start_line, start_col, "identifier too long (>64)");
                }
                program += macro_name_len;

                // seek macro content
                int length = 0;
                int macro_line, macro_col;
                compute_line_col(program_head, program, start_line, start_col, &macro_line, &macro_col);
                Token *macro_content = tokenize_internal(program, &length, source, macro_line, macro_col);
                program += length;

#ifdef DEBUG
                printf("Define Macro [%s]:\n", macro_name);
                printTokens(macro_content);
#endif

                token_hashmap_put(macroMap, macro_name, macro_content);
            } break;
            case '\"': {
                program++;
                char *str = (char*)malloc(256);
                int length = 0;
                while(*program != '\"')
                {
                    if(*program == '\0')
                    {
                        parse_error(source, program_head, program, start_line, start_col, "unterminated string");
                    }
                    str[length++] = *(program++);
                    if(length >= 256)
                    {
                        parse_error(source, program_head, program, start_line, start_col, "string too long (>=256)");
                    }
                }
                program++;//jump the second "
                str[length++] = '\0';
                token->next = newTokenAtCursor(NEW_STRING, source, program_head, program, start_line, start_col);
                token->next->ptr = str;
                token = token->next;
            } break;
            default:
                parse_error(source, program_head, program, start_line, start_col, "unknown character");
        }
    }

    if(pair_pointer > 0)
    {
        parse_error_token(pair_stack[pair_pointer-1], "unclosed '['");
    }
    if(tuple_pointer > 0)
    {
        parse_error_token(tuple_stack[tuple_pointer-1], "unclosed '('");
    }

    *consumed_length = program - program_head;
    return head;
}

Token* tokenize(char *program, int *consumed_length, const char *source)
{
    return tokenize_internal(program, consumed_length, source, 1, 1);
}

#endif
