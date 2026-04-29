#ifndef IR_H
#define IR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

typedef enum IROp IROp;
enum IROp
{
    IR_ADD,
    IR_SUB,
    IR_MOVE_R,
    IR_MOVE_L,
    IR_INPUT,
    IR_OUTPUT,
    IR_JUMP_IF_ZERO,
    IR_JUMP_IF_NONZERO,
    IR_TUPLE_BEGIN,
    IR_TUPLE_END,
    IR_REGION_PUSH,
    IR_REGION_POP,
    IR_NEW_STRING,
    IR_CALL_MACRO,
    IR_CALL_EMBED,
};

typedef struct IRInst IRInst;
struct IRInst
{
    IROp op;
    int num;
    int num_using_stack_top;
    Type type;
    int label;
    int label2;
    char name[64];
    char *text;
    char source[TOKEN_SOURCE_MAX];
    int line;
    int column;
    int scope;
    Token *origin;
    IRInst *next;
};

typedef struct IRLabelRef IRLabelRef;
struct IRLabelRef
{
    Token *token;
    int scope;
    int label;
    IRLabelRef *next;
};

typedef struct IRProgram IRProgram;
struct IRProgram
{
    IRInst *head;
    IRInst *tail;
    IRLabelRef *labels;
    int next_label;
    int next_scope;
};

static inline IRProgram *new_ir_program()
{
    IRProgram *program = (IRProgram*)malloc(sizeof(IRProgram));
    if(!program)
    {
        return 0;
    }
    program->head = 0;
    program->tail = 0;
    program->labels = 0;
    program->next_label = 1;
    program->next_scope = 1;
    return program;
}

static inline IRInst *new_ir_inst(IROp op, Token *token)
{
    IRInst *inst = (IRInst*)malloc(sizeof(IRInst));
    if(!inst)
    {
        return 0;
    }
    inst->op = op;
    inst->num = 0;
    inst->num_using_stack_top = 0;
    inst->type = CHAR;
    inst->label = 0;
    inst->label2 = 0;
    inst->name[0] = '\0';
    inst->text = 0;
    inst->source[0] = '\0';
    inst->line = 0;
    inst->column = 0;
    inst->scope = 0;
    inst->origin = 0;
    inst->next = 0;
    if(token)
    {
        snprintf(inst->source, TOKEN_SOURCE_MAX, "%s", token->source);
        inst->line = token->line;
        inst->column = token->column;
        inst->origin = token;
    }
    return inst;
}

static inline void ir_emit(IRProgram *program, IRInst *inst)
{
    if(!program->head)
    {
        program->head = program->tail = inst;
    }
    else
    {
        program->tail->next = inst;
        program->tail = inst;
    }
}

static inline int ir_get_label(IRProgram *program, Token *token, int scope)
{
    if(!token)
    {
        return 0;
    }
    IRLabelRef *cur = program->labels;
    while(cur)
    {
        if(cur->token == token && cur->scope == scope)
        {
            return cur->label;
        }
        cur = cur->next;
    }

    IRLabelRef *ref = (IRLabelRef*)malloc(sizeof(IRLabelRef));
    if(!ref)
    {
        return 0;
    }
    ref->token = token;
    ref->scope = scope;
    ref->label = program->next_label++;
    ref->next = program->labels;
    program->labels = ref;
    return ref->label;
}

static inline int macro_in_callstack(const char *name, const char **stack, int stack_size)
{
    for(int i = 0; i < stack_size; i++)
    {
        if(strcmp(name, stack[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static inline void generate_ir_from_tokens_internal(
    IRProgram *ir,
    Token *token,
    const char **macro_stack,
    int macro_stack_size,
    int depth,
    int scope
)
{
    if(depth > 64)
    {
        return;
    }

    while(token)
    {
        switch(token->kind)
        {
            case NULLTOK:
                token = token->next;
                break;
            case PLUS: {
                IRInst *inst = new_ir_inst(IR_ADD, token);
                inst->num = token->num;
                inst->num_using_stack_top = token->num_using_stack_top;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case MINUS: {
                IRInst *inst = new_ir_inst(IR_SUB, token);
                inst->num = token->num;
                inst->num_using_stack_top = token->num_using_stack_top;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case RIGHT: {
                IRInst *inst = new_ir_inst(IR_MOVE_R, token);
                inst->num = token->num;
                inst->num_using_stack_top = token->num_using_stack_top;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case LEFT: {
                IRInst *inst = new_ir_inst(IR_MOVE_L, token);
                inst->num = token->num;
                inst->num_using_stack_top = token->num_using_stack_top;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case INPUT: {
                IRInst *inst = new_ir_inst(IR_INPUT, token);
                inst->type = token->type;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case OUTPUT: {
                IRInst *inst = new_ir_inst(IR_OUTPUT, token);
                inst->type = token->type;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case WHEN: {
                IRInst *inst = new_ir_inst(IR_JUMP_IF_ZERO, token);
                inst->label = ir_get_label(ir, token->pair ? token->pair->next : 0, scope);
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case END: {
                IRInst *inst = new_ir_inst(IR_JUMP_IF_NONZERO, token);
                inst->label = ir_get_label(ir, token->pair ? token->pair->next : 0, scope);
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case START_TUPLE: {
                IRInst *inst = new_ir_inst(IR_TUPLE_BEGIN, token);
                inst->num = token->pair ? token->pair->num : 1;
                inst->num_using_stack_top = token->pair ? token->pair->num_using_stack_top : 0;
                inst->label = ir_get_label(ir, token->pair ? token->pair->next : 0, scope);
                inst->label2 = ir_get_label(ir, token->next, scope);
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case END_TUPLE: {
                IRInst *inst = new_ir_inst(IR_TUPLE_END, token);
                inst->label = ir_get_label(ir, token->pair ? token->pair->next : 0, scope);
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case START_REGION: {
                IRInst *inst = new_ir_inst(IR_REGION_PUSH, token);
                inst->num = token->num;
                inst->num_using_stack_top = token->num_using_stack_top;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case END_REGION: {
                IRInst *inst = new_ir_inst(IR_REGION_POP, token);
                inst->num = token->num;
                inst->num_using_stack_top = token->num_using_stack_top;
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case NEW_STRING: {
                IRInst *inst = new_ir_inst(IR_NEW_STRING, token);
                inst->scope = scope;
                if(token->ptr)
                {
                    size_t n = strlen((const char*)token->ptr);
                    inst->text = (char*)malloc(n + 1);
                    if(inst->text)
                    {
                        memcpy(inst->text, token->ptr, n + 1);
                    }
                }
                ir_emit(ir, inst);
                token = token->next;
            } break;
            case MACRO: {
                Token *macro = token_hashmap_get(macroMap, token->macro_name);
                if(macro && macro->kind == EMBED)
                {
                    IRInst *inst = new_ir_inst(IR_CALL_EMBED, token);
                    snprintf(inst->name, sizeof(inst->name), "%s", token->macro_name);
                    inst->scope = scope;
                    ir_emit(ir, inst);
                }
                else if(
                    macro &&
                    !macro_in_callstack(token->macro_name, macro_stack, macro_stack_size)
                )
                {
                    const char *next_stack[128];
                    for(int i = 0; i < macro_stack_size; i++)
                    {
                        next_stack[i] = macro_stack[i];
                    }
                    next_stack[macro_stack_size] = token->macro_name;
                    int child_scope = ir->next_scope++;
                    generate_ir_from_tokens_internal(
                        ir,
                        macro,
                        next_stack,
                        macro_stack_size + 1,
                        depth + 1,
                        child_scope
                    );
                }
                else
                {
                    IRInst *inst = new_ir_inst(IR_CALL_MACRO, token);
                    snprintf(inst->name, sizeof(inst->name), "%s", token->macro_name);
                    inst->scope = scope;
                    ir_emit(ir, inst);
                }
                token = token->next;
            } break;
            case EMBED: {
                IRInst *inst = new_ir_inst(IR_CALL_EMBED, token);
                inst->scope = scope;
                ir_emit(ir, inst);
                token = token->next;
            } break;
        }
    }
}

static inline void generate_ir_from_tokens(IRProgram *ir, Token *token)
{
    const char *macro_stack[128];
    int scope = ir->next_scope++;
    generate_ir_from_tokens_internal(ir, token, macro_stack, 0, 0, scope);
}

static inline int ir_label_for_token(IRProgram *ir, Token *token, int scope)
{
    IRLabelRef *node = ir->labels;
    while(node)
    {
        if(node->token == token && node->scope == scope)
        {
            return node->label;
        }
        node = node->next;
    }
    return 0;
}

static inline int ir_has_unresolved_macro(IRProgram *ir, IRInst **first_macro_call)
{
    IRInst *inst = ir->head;
    while(inst)
    {
        if(inst->op == IR_CALL_MACRO)
        {
            if(first_macro_call)
            {
                *first_macro_call = inst;
            }
            return 1;
        }
        inst = inst->next;
    }
    return 0;
}

static inline const char *ir_type_name(Type type)
{
    switch(type)
    {
        case INTEGER: return "#";
        case STRING: return "%";
        case CHAR:
        default: return "char";
    }
}

static inline void write_num_operand(FILE *out, int num, int using_stack_top)
{
    if(using_stack_top)
    {
        fputs("$top", out);
    }
    else
    {
        fprintf(out, "%d", num);
    }
}

static inline void dump_ir(FILE *out, IRProgram *ir)
{
    IRInst *inst = ir->head;
    while(inst)
    {
        IRLabelRef *node = ir->labels;
        while(node)
        {
            if(node->token != inst->origin || node->scope != inst->scope)
            {
                node = node->next;
                continue;
            }
            fprintf(out, "L%d:\n", node->label);
            node = node->next;
        }

        fprintf(out, "  @ %s:%d:%d ", inst->source[0] ? inst->source : "<unknown>", inst->line, inst->column);
        switch(inst->op)
        {
            case IR_ADD: fputs("ADD ", out); write_num_operand(out, inst->num, inst->num_using_stack_top); break;
            case IR_SUB: fputs("SUB ", out); write_num_operand(out, inst->num, inst->num_using_stack_top); break;
            case IR_MOVE_R: fputs("MOVE_R ", out); write_num_operand(out, inst->num, inst->num_using_stack_top); break;
            case IR_MOVE_L: fputs("MOVE_L ", out); write_num_operand(out, inst->num, inst->num_using_stack_top); break;
            case IR_INPUT: fprintf(out, "INPUT %s", ir_type_name(inst->type)); break;
            case IR_OUTPUT: fprintf(out, "OUTPUT %s", ir_type_name(inst->type)); break;
            case IR_JUMP_IF_ZERO: fprintf(out, "JZ L%d", inst->label); break;
            case IR_JUMP_IF_NONZERO: fprintf(out, "JNZ L%d", inst->label); break;
            case IR_TUPLE_BEGIN:
                fputs("TUPLE_BEGIN ", out);
                write_num_operand(out, inst->num, inst->num_using_stack_top);
                fprintf(out, " BODY=L%d END=L%d", inst->label2, inst->label);
                break;
            case IR_TUPLE_END: fprintf(out, "TUPLE_END BODY=L%d", inst->label); break;
            case IR_REGION_PUSH: fputs("REGION_PUSH ", out); write_num_operand(out, inst->num, inst->num_using_stack_top); break;
            case IR_REGION_POP: fputs("REGION_POP ", out); write_num_operand(out, inst->num, inst->num_using_stack_top); break;
            case IR_NEW_STRING: fprintf(out, "NEW_STRING \"%s\"", inst->text ? inst->text : ""); break;
            case IR_CALL_MACRO: fprintf(out, "CALL %s", inst->name); break;
            case IR_CALL_EMBED: fprintf(out, "CALL_EMBED %s", inst->name); break;
        }
        fputc('\n', out);

        inst = inst->next;
    }
}

static inline char *default_ir_output_name(const char *input_path)
{
    const char *base = input_path;
    for(const char *p = input_path; *p; p++)
    {
        if(*p == '\\' || *p == '/')
        {
            base = p + 1;
        }
    }

    const char *dot = strrchr(base, '.');
    size_t base_len = dot ? (size_t)(dot - base) : strlen(base);
    if(base_len == 0)
    {
        base_len = strlen(base);
    }

    char *output = (char*)malloc(base_len + 4);
    if(!output)
    {
        return 0;
    }
    memcpy(output, base, base_len);
    memcpy(output + base_len, ".ir", 4);
    return output;
}

static inline int write_ir_file(const char *output_path, Token *tokens)
{
    IRProgram *ir = new_ir_program();
    if(!ir)
    {
        return 1;
    }

    generate_ir_from_tokens(ir, tokens);

    FILE *out = fopen(output_path, "wb");
    if(!out)
    {
        perror("Failed to open IR output");
        return 1;
    }
    dump_ir(out, ir);
    fclose(out);
    return 0;
}

#endif
