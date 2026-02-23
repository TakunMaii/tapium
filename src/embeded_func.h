#ifndef EMBEDED_FUNC_H
#define EMBEDED_FUNC_H

#include "hashmap.h"
#include "simulate.h"
#include "token.h"

void tap_get_rand()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    tape[pointer] = rand();
}

void tap_push()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    stack[stack_pointer] = tape[pointer];
    region_stack[region_pointer-1]->stack_pointer++;
}

void tap_pop()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = stack[stack_pointer-1];
    region_stack[region_pointer-1]->stack_pointer--;
}

void tap_peek()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = stack[stack_pointer-1];
}

void tap_add()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] + stack[stack_pointer-1];
}

void tap_sub()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] - stack[stack_pointer-1];
}

void tap_mul()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] * stack[stack_pointer-1];
}

void tap_div()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] / stack[stack_pointer-1];
}

void tap_eq()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] == stack[stack_pointer-1];
}

void tap_ineq()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] != stack[stack_pointer-1];
}

void tap_greater()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] > stack[stack_pointer-1];
}

void tap_less()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] < stack[stack_pointer-1];
}

void tap_greater_eq()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] >= stack[stack_pointer-1];
}

void tap_less_eq()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = tape[pointer] <= stack[stack_pointer-1];
}

void tap_mem()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    tape[pointer] = (long long)malloc(stack[stack_pointer-1]);
}

void tap_free()
{
    long long *tape = region_stack[region_pointer-1]->tape;
    int pointer = region_stack[region_pointer-1]->pointer;
    long long *stack = region_stack[region_pointer-1]->stack;
    int stack_pointer = region_stack[region_pointer-1]->stack_pointer;
    free((void*)tape[pointer]);
}

void register_embeded_func()
{
    Token *token = newToken(EMBED);
    token->embedded_func = tap_get_rand;
    token_hashmap_put(macroMap, "rand", token);

    token = newToken(EMBED);
    token->embedded_func = tap_add;
    token_hashmap_put(macroMap, "add", token);

    token = newToken(EMBED);
    token->embedded_func = tap_sub;
    token_hashmap_put(macroMap, "sub", token);

    token = newToken(EMBED);
    token->embedded_func = tap_mul;
    token_hashmap_put(macroMap, "mul", token);

    token = newToken(EMBED);
    token->embedded_func = tap_div;
    token_hashmap_put(macroMap, "div", token);

    token = newToken(EMBED);
    token->embedded_func = tap_eq;
    token_hashmap_put(macroMap, "eq", token);

    token = newToken(EMBED);
    token->embedded_func = tap_ineq;
    token_hashmap_put(macroMap, "ineq", token);

    token = newToken(EMBED);
    token->embedded_func = tap_greater;
    token_hashmap_put(macroMap, "greater", token);

    token = newToken(EMBED);
    token->embedded_func = tap_less;
    token_hashmap_put(macroMap, "less", token);

    token = newToken(EMBED);
    token->embedded_func = tap_greater_eq;
    token_hashmap_put(macroMap, "greater_eq", token);

    token = newToken(EMBED);
    token->embedded_func = tap_less_eq;
    token_hashmap_put(macroMap, "less_eq", token);

    token = newToken(EMBED);
    token->embedded_func = tap_push;
    token_hashmap_put(macroMap, "push", token);

    token = newToken(EMBED);
    token->embedded_func = tap_pop;
    token_hashmap_put(macroMap, "pop", token);

    token = newToken(EMBED);
    token->embedded_func = tap_peek;
    token_hashmap_put(macroMap, "peek", token);

    token = newToken(EMBED);
    token->embedded_func = tap_mem;
    token_hashmap_put(macroMap, "mem", token);

    token = newToken(EMBED);
    token->embedded_func = tap_free;
    token_hashmap_put(macroMap, "free", token);
}


#endif
