#ifndef COMPILE_H
#define COMPILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "file.h"
#include "ir.h"
#include "embeded_func.h"

typedef struct StringBuilder {
    char *buf;
    size_t len;
    size_t cap;
} StringBuilder;

static inline int sb_init(StringBuilder *sb)
{
    sb->cap = 1024;
    sb->len = 0;
    sb->buf = (char*)malloc(sb->cap);
    if (!sb->buf) {
        return 0;
    }
    sb->buf[0] = '\0';
    return 1;
}

static inline int sb_ensure(StringBuilder *sb, size_t extra)
{
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap) {
        return 1;
    }
    while (sb->cap < need) {
        sb->cap *= 2;
    }
    char *next = (char*)realloc(sb->buf, sb->cap);
    if (!next) {
        return 0;
    }
    sb->buf = next;
    return 1;
}

static inline int sb_append_char(StringBuilder *sb, char c)
{
    if (!sb_ensure(sb, 1)) {
        return 0;
    }
    sb->buf[sb->len++] = c;
    sb->buf[sb->len] = '\0';
    return 1;
}

static inline int sb_append_str(StringBuilder *sb, const char *s)
{
    size_t n = strlen(s);
    if (!sb_ensure(sb, n)) {
        return 0;
    }
    memcpy(sb->buf + sb->len, s, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
    return 1;
}

static inline int is_absolute_path(const char *path)
{
    if (!path || !path[0]) {
        return 0;
    }
    if (path[0] == '\\' || path[0] == '/') {
        return 1;
    }
    if (path[0] && path[1] == ':') {
        return 1;
    }
    return 0;
}

static inline char *dir_of_path(const char *path)
{
    const char *last_sep = 0;
    for (const char *p = path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            last_sep = p;
        }
    }
    if (!last_sep) {
        char *cur = (char*)malloc(2);
        if (!cur) {
            return 0;
        }
        cur[0] = '.';
        cur[1] = '\0';
        return cur;
    }

    size_t len = (size_t)(last_sep - path);
    char *dir = (char*)malloc(len + 1);
    if (!dir) {
        return 0;
    }
    memcpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

static inline char *join_path(const char *base_dir, const char *rel)
{
    if (is_absolute_path(rel)) {
        char *out = (char*)malloc(strlen(rel) + 1);
        if (!out) {
            return 0;
        }
        strcpy(out, rel);
        return out;
    }

    size_t base_len = strlen(base_dir);
    size_t rel_len = strlen(rel);
    int need_sep = base_len > 0 && base_dir[base_len - 1] != '\\' && base_dir[base_len - 1] != '/';
    char *out = (char*)malloc(base_len + (need_sep ? 1 : 0) + rel_len + 1);
    if (!out) {
        return 0;
    }
    strcpy(out, base_dir);
    if (need_sep) {
        strcat(out, "\\");
    }
    strcat(out, rel);
    return out;
}

static inline int path_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    fclose(f);
    return 1;
}

static inline int expand_source_with_includes(
    const char *source_path,
    int depth,
    StringBuilder *out
)
{
    if (depth > 32) {
        printf("Build error: include nesting too deep near %s\n", source_path);
        return 0;
    }

    long len = 0;
    char *content = read_file(source_path, &len);
    if (!content) {
        printf("Build error: failed to read %s\n", source_path);
        return 0;
    }

    char *base_dir = dir_of_path(source_path);
    if (!base_dir) {
        free(content);
        return 0;
    }

    const char *p = content;
    int line_start = 1;
    while (*p) {
        if (line_start && *p == '%') {
            p++;
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            const char *path_start = p;
            while (*p && *p != '\n' && *p != '\r') {
                p++;
            }
            const char *path_end = p;
            while (path_end > path_start && isspace((unsigned char)path_end[-1])) {
                path_end--;
            }

            size_t path_len = (size_t)(path_end - path_start);
            char *include_rel = (char*)malloc(path_len + 1);
            if (!include_rel) {
                free(base_dir);
                free(content);
                return 0;
            }
            memcpy(include_rel, path_start, path_len);
            include_rel[path_len] = '\0';

            char *include_abs = join_path(base_dir, include_rel);
            free(include_rel);
            if (!include_abs) {
                free(base_dir);
                free(content);
                return 0;
            }

            if (!path_exists(include_abs)) {
                free(include_abs);
                include_abs = (char*)malloc(path_len + 1);
                if (!include_abs) {
                    free(base_dir);
                    free(content);
                    return 0;
                }
                memcpy(include_abs, path_start, path_len);
                include_abs[path_len] = '\0';
            }

            if (!expand_source_with_includes(include_abs, depth + 1, out)) {
                free(include_abs);
                free(base_dir);
                free(content);
                return 0;
            }
            free(include_abs);

            if (*p == '\r') {
                p++;
            }
            if (*p == '\n') {
                if (!sb_append_char(out, '\n')) {
                    free(base_dir);
                    free(content);
                    return 0;
                }
                p++;
            }
            line_start = 1;
            continue;
        }

        if (!sb_append_char(out, *p)) {
            free(base_dir);
            free(content);
            return 0;
        }
        line_start = (*p == '\n');
        p++;
    }

    free(base_dir);
    free(content);
    return 1;
}

static inline char *load_expanded_program(const char *input_path)
{
    StringBuilder sb;
    if (!sb_init(&sb)) {
        return 0;
    }
    if (!expand_source_with_includes(input_path, 0, &sb)) {
        free(sb.buf);
        return 0;
    }
    return sb.buf;
}

static inline void write_c_string_literal(FILE *file, const char *text)
{
    fputc('"', file);
    while (*text) {
        unsigned char c = (unsigned char)*text++;
        switch (c) {
            case '\\': fputs("\\\\", file); break;
            case '"':  fputs("\\\"", file); break;
            case '\n': fputs("\\n", file); break;
            case '\r': fputs("\\r", file); break;
            case '\t': fputs("\\t", file); break;
            default:
                if (c < 32 || c > 126) {
                    fprintf(file, "\\%03o", c);
                } else {
                    fputc(c, file);
                }
                break;
        }
    }
    fputc('"', file);
}

static inline char *default_output_name(const char *input_path)
{
    const char *base = input_path;
    for (const char *p = input_path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }

    const char *dot = strrchr(base, '.');
    size_t base_len = dot ? (size_t)(dot - base) : strlen(base);
    if (base_len == 0) {
        base_len = strlen(base);
    }

    char *output = (char*)malloc(base_len + 5);
    if (!output) {
        return NULL;
    }

    memcpy(output, base, base_len);
    memcpy(output + base_len, ".exe", 5);
    return output;
}

static inline char *default_c_output_name(const char *input_path)
{
    const char *base = input_path;
    for (const char *p = input_path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }

    const char *dot = strrchr(base, '.');
    size_t base_len = dot ? (size_t)(dot - base) : strlen(base);
    if (base_len == 0) {
        base_len = strlen(base);
    }

    char *output = (char*)malloc(base_len + 3);
    if (!output) {
        return NULL;
    }

    memcpy(output, base, base_len);
    memcpy(output + base_len, ".c", 3);
    return output;
}

static inline char *default_asm_output_name(const char *input_path)
{
    const char *base = input_path;
    for (const char *p = input_path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }

    const char *dot = strrchr(base, '.');
    size_t base_len = dot ? (size_t)(dot - base) : strlen(base);
    if (base_len == 0) {
        base_len = strlen(base);
    }

    char *output = (char*)malloc(base_len + 3);
    if (!output) {
        return NULL;
    }

    memcpy(output, base, base_len);
    memcpy(output + base_len, ".s", 3);
    return output;
}

static inline int ir_instruction_count(IRProgram *ir)
{
    int count = 0;
    IRInst *inst = ir->head;
    while(inst)
    {
        count++;
        inst = inst->next;
    }
    return count;
}

static inline const char *embedded_name_to_c_func(const char *name)
{
    if(strcmp(name, "rand") == 0) return "tap_get_rand";
    if(strcmp(name, "push") == 0) return "tap_push";
    if(strcmp(name, "pop") == 0) return "tap_pop";
    if(strcmp(name, "peek") == 0) return "tap_peek";
    if(strcmp(name, "add") == 0) return "tap_add";
    if(strcmp(name, "sub") == 0) return "tap_sub";
    if(strcmp(name, "mul") == 0) return "tap_mul";
    if(strcmp(name, "div") == 0) return "tap_div";
    if(strcmp(name, "greater") == 0) return "tap_greater";
    if(strcmp(name, "less") == 0) return "tap_less";
    if(strcmp(name, "greater_eq") == 0) return "tap_greater_eq";
    if(strcmp(name, "less_eq") == 0) return "tap_less_eq";
    if(strcmp(name, "eq") == 0) return "tap_eq";
    if(strcmp(name, "ineq") == 0) return "tap_ineq";
    if(strcmp(name, "mem") == 0) return "tap_mem";
    if(strcmp(name, "free") == 0) return "tap_free";
    return 0;
}

static inline void emit_codegen_headers(FILE *out)
{
    fputs("#include <stdlib.h>\n", out);
    fputs("#include <string.h>\n", out);
    fputs("#include <stdio.h>\n", out);
    fputs("#include \"src/hashmap.h\"\n", out);
    fputs("#include \"src/simulate.h\"\n", out);
    fputs("#include \"src/token.h\"\n", out);
    fputs("#include \"src/embeded_func.h\"\n\n", out);
}

static inline void emit_ir_metadata_array(FILE *out, IRProgram *ir, int inst_count)
{
    fputs("static Token IR_META[] = {\n", out);
    IRInst *inst = ir->head;
    while(inst)
    {
        fputs("    { .source = ", out);
        write_c_string_literal(out, inst->source[0] ? inst->source : "<unknown>");
        fprintf(out, ", .line = %d, .column = %d },\n", inst->line, inst->column);
        inst = inst->next;
    }
    if(inst_count == 0)
    {
        fputs("    {0},\n", out);
    }
    fputs("};\n\n", out);
}

static inline void emit_ir_runtime_helpers(FILE *out)
{
    fputs("static int ir_resolve_num(int num, int use_stack_top)\n", out);
    fputs("{\n", out);
    fputs("    if(use_stack_top)\n", out);
    fputs("    {\n", out);
    fputs("        ensure_stack_not_empty();\n", out);
    fputs("        return STACK[STPTR-1];\n", out);
    fputs("    }\n", out);
    fputs("    return num;\n", out);
    fputs("}\n\n", out);
}

static inline void emit_instruction_c(FILE *out, IRInst *inst)
{
    switch(inst->op)
    {
        case IR_ADD:
            fprintf(out, "    ensure_pointer_in_bounds(POINTER); TAPE[POINTER] += ir_resolve_num(%d, %d);\n", inst->num, inst->num_using_stack_top);
            break;
        case IR_SUB:
            fprintf(out, "    ensure_pointer_in_bounds(POINTER); TAPE[POINTER] -= ir_resolve_num(%d, %d);\n", inst->num, inst->num_using_stack_top);
            break;
        case IR_MOVE_R:
            fprintf(out, "    POINTER += ir_resolve_num(%d, %d); ensure_pointer_in_bounds(POINTER);\n", inst->num, inst->num_using_stack_top);
            break;
        case IR_MOVE_L:
            fprintf(out, "    POINTER -= ir_resolve_num(%d, %d); ensure_pointer_in_bounds(POINTER);\n", inst->num, inst->num_using_stack_top);
            break;
        case IR_INPUT:
            fputs("    ensure_pointer_in_bounds(POINTER);\n", out);
            if(inst->type == INTEGER)
            {
                fputs("    scanf(\"%lld\", &TAPE[POINTER]);\n", out);
            }
            else if(inst->type == STRING)
            {
                fputs("    { char *str = (char*)malloc(256); if(!str) runtime_error(\"malloc failed\"); scanf(\"%255s\", str); TAPE[POINTER] = (long long)str; }\n", out);
            }
            else
            {
                fputs("    scanf(\"%c\", (char*)&TAPE[POINTER]);\n", out);
            }
            break;
        case IR_OUTPUT:
            fputs("    ensure_pointer_in_bounds(POINTER);\n", out);
            if(inst->type == INTEGER)
            {
                fputs("    printf(\"%lld\", TAPE[POINTER]);\n", out);
            }
            else if(inst->type == STRING)
            {
                fputs("    printf(\"%s\", (char*)TAPE[POINTER]);\n", out);
            }
            else
            {
                fputs("    printf(\"%c\", (int)TAPE[POINTER]);\n", out);
            }
            break;
        case IR_JUMP_IF_ZERO:
            if(inst->label == 0)
            {
                fputs("    ensure_pointer_in_bounds(POINTER); if(!TAPE[POINTER]) goto L_END;\n", out);
            }
            else
            {
                fprintf(out, "    ensure_pointer_in_bounds(POINTER); if(!TAPE[POINTER]) goto L%d;\n", inst->label);
            }
            break;
        case IR_JUMP_IF_NONZERO:
            if(inst->label == 0)
            {
                fputs("    ensure_pointer_in_bounds(POINTER); if(TAPE[POINTER]) goto L_END;\n", out);
            }
            else
            {
                fprintf(out, "    ensure_pointer_in_bounds(POINTER); if(TAPE[POINTER]) goto L%d;\n", inst->label);
            }
            break;
        case IR_TUPLE_BEGIN:
            fputs("    if(tuple_times_pointer >= 128) runtime_error(\"tuple nesting too deep\");\n", out);
            fprintf(out, "    tuple_times_stack[tuple_times_pointer++] = ir_resolve_num(%d, %d);\n", inst->num, inst->num_using_stack_top);
            if(inst->label == 0)
            {
                fputs("    if(tuple_times_stack[tuple_times_pointer-1] <= 0) { tuple_times_pointer--; goto L_END; }\n", out);
            }
            else
            {
                fprintf(out, "    if(tuple_times_stack[tuple_times_pointer-1] <= 0) { tuple_times_pointer--; goto L%d; }\n", inst->label);
            }
            break;
        case IR_TUPLE_END:
            fputs("    if(tuple_times_pointer <= 0) runtime_error(\"tuple stack underflow\");\n", out);
            if(inst->label == 0)
            {
                fputs("    if(--tuple_times_stack[tuple_times_pointer-1]) goto L_END; else tuple_times_pointer--;\n", out);
            }
            else
            {
                fprintf(out, "    if(--tuple_times_stack[tuple_times_pointer-1]) goto L%d; else tuple_times_pointer--;\n", inst->label);
            }
            break;
        case IR_REGION_PUSH:
            fprintf(out, "    pushRegionWith(ir_resolve_num(%d, %d));\n", inst->num, inst->num_using_stack_top);
            break;
        case IR_REGION_POP:
            fprintf(out, "    popRegionWith(ir_resolve_num(%d, %d));\n", inst->num, inst->num_using_stack_top);
            break;
        case IR_NEW_STRING:
            fputs("    ensure_pointer_in_bounds(POINTER);\n", out);
            fputs("    {\n", out);
            fputs("        char *tap_str = (char*)malloc(", out);
            fprintf(out, "%zu", inst->text ? strlen(inst->text) + 1 : (size_t)1);
            fputs(");\n", out);
            fputs("        if(!tap_str) runtime_error(\"malloc failed\");\n", out);
            fputs("        strcpy(tap_str, ", out);
            write_c_string_literal(out, inst->text ? inst->text : "");
            fputs(");\n", out);
            fputs("        TAPE[POINTER] = (long long)tap_str;\n", out);
            fputs("    }\n", out);
            break;
        case IR_CALL_EMBED: {
            const char *func = embedded_name_to_c_func(inst->name);
            if(func)
            {
                fprintf(out, "    %s();\n", func);
            }
            else
            {
                fputs("    runtime_error(\"unknown embedded function in IR\");\n", out);
            }
        } break;
        case IR_CALL_MACRO:
            fputs("    runtime_error(\"unresolved macro call in compiled IR backend\");\n", out);
            break;
    }
}

static inline int emit_ir_c_program(const char *generated_c, IRProgram *ir)
{
    FILE *out = fopen(generated_c, "wb");
    if(!out)
    {
        perror("Failed to create generated C file");
        return 0;
    }

    int inst_count = ir_instruction_count(ir);
    emit_codegen_headers(out);
    emit_ir_metadata_array(out, ir, inst_count);
    emit_ir_runtime_helpers(out);
    fputs("int main(void)\n{\n", out);
    fputs("    int tuple_times_stack[128];\n", out);
    fputs("    int tuple_times_pointer = 0;\n", out);
    fputs("    pushRegionWith(0);\n", out);

    int index = 0;
    IRInst *inst = ir->head;
    while(inst)
    {
        int label = ir_label_for_token(ir, inst->origin, inst->scope);
        if(label > 0)
        {
            fprintf(out, "L%d:\n", label);
        }
        fprintf(out, "    runtime_current_token = &IR_META[%d];\n", index);
        emit_instruction_c(out, inst);
        inst = inst->next;
        index++;
    }

    fputs("L_END:\n", out);
    fputs("    return 0;\n", out);
    fputs("}\n", out);
    fclose(out);
    return 1;
}

static inline int prepare_ir_from_input(const char *input_path, char **program_out, IRProgram **ir_out)
{
    char *program = load_expanded_program(input_path);
    if (!program) {
        return 1;
    }

    macroMap = token_hashmap_create(127);
    if(!macroMap)
    {
        free(program);
        return 1;
    }
    register_embeded_func();
    int length = 0;
    Token *tokens = tokenize(program, &length, input_path);
    IRProgram *ir = new_ir_program();
    if(!ir)
    {
        free(program);
        return 1;
    }
    generate_ir_from_tokens(ir, tokens);

    IRInst *first_macro_call = 0;
    if(ir_has_unresolved_macro(ir, &first_macro_call))
    {
        printf(
            "Build error at %s:%d:%d: unresolved macro call '%s' (likely recursive macro, backend requires inlining)\n",
            first_macro_call->source[0] ? first_macro_call->source : "<unknown>",
            first_macro_call->line,
            first_macro_call->column,
            first_macro_call->name
        );
        free(program);
        return 1;
    }

    *program_out = program;
    *ir_out = ir;
    return 0;
}

static inline int compile_to_c_source(const char *input_path, const char *output_c_path)
{
    char *program = 0;
    IRProgram *ir = 0;
    if(prepare_ir_from_input(input_path, &program, &ir) != 0)
    {
        return 1;
    }

    if(!emit_ir_c_program(output_c_path, ir))
    {
        free(program);
        return 1;
    }

    free(program);
    return 0;
}

static inline void write_asm_string_literal(FILE *out, const char *text)
{
    fputc('"', out);
    while(*text)
    {
        unsigned char c = (unsigned char)*text++;
        switch(c)
        {
            case '\\': fputs("\\\\", out); break;
            case '"': fputs("\\\"", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if(c < 32 || c > 126)
                {
                    fprintf(out, "\\%03o", c);
                }
                else
                {
                    fputc(c, out);
                }
                break;
        }
    }
    fputc('"', out);
}

static inline int ir_contains_region_ops(IRProgram *ir, IRInst **inst_out)
{
    IRInst *inst = ir->head;
    while(inst)
    {
        if(inst->op == IR_REGION_PUSH || inst->op == IR_REGION_POP)
        {
            if(inst_out) *inst_out = inst;
            return 1;
        }
        inst = inst->next;
    }
    return 0;
}

static inline int ir_contains_unknown_embed(IRProgram *ir, IRInst **inst_out)
{
    IRInst *inst = ir->head;
    while(inst)
    {
        if(inst->op == IR_CALL_EMBED)
        {
            if(!embedded_name_to_c_func(inst->name))
            {
                if(inst_out) *inst_out = inst;
                return 1;
            }
        }
        inst = inst->next;
    }
    return 0;
}

static inline void emit_asm_pointer_bounds_check(FILE *out)
{
    fputs("    mov rax, qword ptr [rip + pointer]\n", out);
    fputs("    cmp rax, 0\n", out);
    fputs("    jl L_ERR_PTR\n", out);
    fputs("    cmp rax, 1023\n", out);
    fputs("    jg L_ERR_PTR\n", out);
}

static inline void emit_asm_resolve_num(FILE *out, int num, int use_stack_top)
{
    if(use_stack_top)
    {
        fputs("    mov rax, qword ptr [rip + tap_sp]\n", out);
        fputs("    cmp rax, 0\n", out);
        fputs("    jle L_ERR_STACK_UNDER\n", out);
        fputs("    dec rax\n", out);
        fputs("    lea rbx, [rip + tap_stack]\n", out);
        fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
    }
    else
    {
        fprintf(out, "    mov rdx, %d\n", num);
    }
}

static inline void emit_asm_instruction(FILE *out, IRInst *inst, int string_id)
{
    switch(inst->op)
    {
        case IR_ADD:
            emit_asm_resolve_num(out, inst->num, inst->num_using_stack_top);
            emit_asm_pointer_bounds_check(out);
            fputs("    mov rax, qword ptr [rip + pointer]\n", out);
            fputs("    lea rbx, [rip + tape]\n", out);
            fputs("    add qword ptr [rbx + rax*8], rdx\n", out);
            break;
        case IR_SUB:
            emit_asm_resolve_num(out, inst->num, inst->num_using_stack_top);
            emit_asm_pointer_bounds_check(out);
            fputs("    mov rax, qword ptr [rip + pointer]\n", out);
            fputs("    lea rbx, [rip + tape]\n", out);
            fputs("    sub qword ptr [rbx + rax*8], rdx\n", out);
            break;
        case IR_MOVE_R:
            emit_asm_resolve_num(out, inst->num, inst->num_using_stack_top);
            fputs("    add qword ptr [rip + pointer], rdx\n", out);
            emit_asm_pointer_bounds_check(out);
            break;
        case IR_MOVE_L:
            emit_asm_resolve_num(out, inst->num, inst->num_using_stack_top);
            fputs("    sub qword ptr [rip + pointer], rdx\n", out);
            emit_asm_pointer_bounds_check(out);
            break;
        case IR_INPUT:
            emit_asm_pointer_bounds_check(out);
            if(inst->type == INTEGER)
            {
                fputs("    lea rcx, [rip + FMT_INT_IN]\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    lea rdx, [rbx + rax*8]\n", out);
                fputs("    call scanf\n", out);
            }
            else if(inst->type == STRING)
            {
                fputs("    mov rcx, 256\n", out);
                fputs("    call malloc\n", out);
                fputs("    test rax, rax\n", out);
                fputs("    jz L_ERR_MALLOC\n", out);
                fputs("    mov r8, rax\n", out);
                fputs("    lea rcx, [rip + FMT_STR_IN]\n", out);
                fputs("    mov rdx, r8\n", out);
                fputs("    call scanf\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov qword ptr [rbx + rax*8], r8\n", out);
            }
            else
            {
                fputs("    lea rcx, [rip + FMT_CHR_IN]\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    lea rdx, [rbx + rax*8]\n", out);
                fputs("    call scanf\n", out);
            }
            break;
        case IR_OUTPUT:
            emit_asm_pointer_bounds_check(out);
            if(inst->type == INTEGER)
            {
                fputs("    lea rcx, [rip + FMT_INT_OUT]\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
                fputs("    call printf\n", out);
            }
            else if(inst->type == STRING)
            {
                fputs("    lea rcx, [rip + FMT_STR_OUT]\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
                fputs("    call printf\n", out);
            }
            else
            {
                fputs("    lea rcx, [rip + FMT_CHR_OUT]\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
                fputs("    call printf\n", out);
            }
            break;
        case IR_JUMP_IF_ZERO:
            emit_asm_pointer_bounds_check(out);
            fputs("    mov rax, qword ptr [rip + pointer]\n", out);
            fputs("    lea rbx, [rip + tape]\n", out);
            fputs("    cmp qword ptr [rbx + rax*8], 0\n", out);
            if(inst->label == 0) fputs("    je L_END\n", out);
            else fprintf(out, "    je L%d\n", inst->label);
            break;
        case IR_JUMP_IF_NONZERO:
            emit_asm_pointer_bounds_check(out);
            fputs("    mov rax, qword ptr [rip + pointer]\n", out);
            fputs("    lea rbx, [rip + tape]\n", out);
            fputs("    cmp qword ptr [rbx + rax*8], 0\n", out);
            if(inst->label == 0) fputs("    jne L_END\n", out);
            else fprintf(out, "    jne L%d\n", inst->label);
            break;
        case IR_TUPLE_BEGIN:
            emit_asm_resolve_num(out, inst->num, inst->num_using_stack_top);
            fputs("    mov rax, qword ptr [rip + tuple_sp]\n", out);
            fputs("    cmp rax, 128\n", out);
            fputs("    jge L_ERR_TUPLE_OVER\n", out);
            fputs("    lea rbx, [rip + tuple_stack]\n", out);
            fputs("    mov qword ptr [rbx + rax*8], rdx\n", out);
            fputs("    inc qword ptr [rip + tuple_sp]\n", out);
            fputs("    cmp rdx, 0\n", out);
            if(inst->label == 0) fputs("    jle L_END\n", out);
            else fprintf(out, "    jle L%d\n", inst->label);
            break;
        case IR_TUPLE_END:
            fputs("    mov rax, qword ptr [rip + tuple_sp]\n", out);
            fputs("    cmp rax, 0\n", out);
            fputs("    jle L_ERR_TUPLE_UNDER\n", out);
            fputs("    dec rax\n", out);
            fputs("    lea rbx, [rip + tuple_stack]\n", out);
            fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
            fputs("    dec rdx\n", out);
            fputs("    mov qword ptr [rbx + rax*8], rdx\n", out);
            fputs("    cmp rdx, 0\n", out);
            if(inst->label == 0) fputs("    jne L_END\n", out);
            else fprintf(out, "    jne L%d\n", inst->label);
            fputs("    mov qword ptr [rip + tuple_sp], rax\n", out);
            break;
        case IR_REGION_PUSH:
        case IR_REGION_POP:
            fputs("    jmp L_ERR_REGION_UNSUPPORTED\n", out);
            break;
        case IR_NEW_STRING:
            emit_asm_pointer_bounds_check(out);
            fprintf(out, "    mov rcx, %zu\n", inst->text ? strlen(inst->text) + 1 : (size_t)1);
            fputs("    call malloc\n", out);
            fputs("    test rax, rax\n", out);
            fputs("    jz L_ERR_MALLOC\n", out);
            fputs("    mov r8, rax\n", out);
            fputs("    mov rcx, r8\n", out);
            fprintf(out, "    lea rdx, [rip + STR_%d]\n", string_id);
            fputs("    call strcpy\n", out);
            fputs("    mov rax, qword ptr [rip + pointer]\n", out);
            fputs("    lea rbx, [rip + tape]\n", out);
            fputs("    mov qword ptr [rbx + rax*8], r8\n", out);
            break;
        case IR_CALL_EMBED: {
            const char *name = inst->name;
            if(strcmp(name, "rand") == 0)
            {
                fputs("    call rand\n", out);
                emit_asm_pointer_bounds_check(out);
                fputs("    movsx rdx, eax\n", out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov qword ptr [rbx + rax*8], rdx\n", out);
            }
            else if(strcmp(name, "push") == 0)
            {
                fputs("    mov rax, qword ptr [rip + tap_sp]\n", out);
                fputs("    cmp rax, 1024\n", out);
                fputs("    jge L_ERR_STACK_OVER\n", out);
                emit_asm_pointer_bounds_check(out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov rcx, qword ptr [rip + pointer]\n", out);
                fputs("    mov rdx, qword ptr [rbx + rcx*8]\n", out);
                fputs("    lea rbx, [rip + tap_stack]\n", out);
                fputs("    mov qword ptr [rbx + rax*8], rdx\n", out);
                fputs("    inc qword ptr [rip + tap_sp]\n", out);
            }
            else if(strcmp(name, "pop") == 0 || strcmp(name, "peek") == 0)
            {
                fputs("    mov rax, qword ptr [rip + tap_sp]\n", out);
                fputs("    cmp rax, 0\n", out);
                fputs("    jle L_ERR_STACK_UNDER\n", out);
                fputs("    dec rax\n", out);
                fputs("    lea rbx, [rip + tap_stack]\n", out);
                fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
                emit_asm_pointer_bounds_check(out);
                fputs("    mov rcx, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov qword ptr [rbx + rcx*8], rdx\n", out);
                if(strcmp(name, "pop") == 0)
                {
                    fputs("    mov qword ptr [rip + tap_sp], rax\n", out);
                }
            }
            else if(strcmp(name, "mem") == 0)
            {
                fputs("    mov rax, qword ptr [rip + tap_sp]\n", out);
                fputs("    cmp rax, 0\n", out);
                fputs("    jle L_ERR_STACK_UNDER\n", out);
                fputs("    dec rax\n", out);
                fputs("    lea rbx, [rip + tap_stack]\n", out);
                fputs("    mov rcx, qword ptr [rbx + rax*8]\n", out);
                fputs("    cmp rcx, 0\n", out);
                fputs("    jle L_ERR_MEM_SIZE\n", out);
                fputs("    call malloc\n", out);
                fputs("    test rax, rax\n", out);
                fputs("    jz L_ERR_MALLOC\n", out);
                emit_asm_pointer_bounds_check(out);
                fputs("    mov rcx, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov qword ptr [rbx + rcx*8], rax\n", out);
            }
            else if(strcmp(name, "free") == 0)
            {
                emit_asm_pointer_bounds_check(out);
                fputs("    mov rax, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov rcx, qword ptr [rbx + rax*8]\n", out);
                fputs("    call free\n", out);
            }
            else
            {
                int is_div = strcmp(name, "div") == 0;
                int is_add = strcmp(name, "add") == 0;
                int is_sub = strcmp(name, "sub") == 0;
                int is_mul = strcmp(name, "mul") == 0;
                int is_eq = strcmp(name, "eq") == 0;
                int is_ineq = strcmp(name, "ineq") == 0;
                int is_gt = strcmp(name, "greater") == 0;
                int is_lt = strcmp(name, "less") == 0;
                int is_ge = strcmp(name, "greater_eq") == 0;
                int is_le = strcmp(name, "less_eq") == 0;
                fputs("    mov rax, qword ptr [rip + tap_sp]\n", out);
                fputs("    cmp rax, 0\n", out);
                fputs("    jle L_ERR_STACK_UNDER\n", out);
                fputs("    dec rax\n", out);
                fputs("    lea rbx, [rip + tap_stack]\n", out);
                fputs("    mov rdx, qword ptr [rbx + rax*8]\n", out);
                emit_asm_pointer_bounds_check(out);
                fputs("    mov rcx, qword ptr [rip + pointer]\n", out);
                fputs("    lea rbx, [rip + tape]\n", out);
                fputs("    mov rax, qword ptr [rbx + rcx*8]\n", out);
                if(is_add) fputs("    add rax, rdx\n", out);
                else if(is_sub) fputs("    sub rax, rdx\n", out);
                else if(is_mul) fputs("    imul rax, rdx\n", out);
                else if(is_div)
                {
                    fputs("    mov r9, rdx\n", out);
                    fputs("    cmp r9, 0\n", out);
                    fputs("    je L_ERR_DIV_ZERO\n", out);
                    fputs("    cqo\n", out);
                    fputs("    idiv r9\n", out);
                }
                else
                {
                    fputs("    cmp rax, rdx\n", out);
                    if(is_eq) fputs("    sete al\n", out);
                    else if(is_ineq) fputs("    setne al\n", out);
                    else if(is_gt) fputs("    setg al\n", out);
                    else if(is_lt) fputs("    setl al\n", out);
                    else if(is_ge) fputs("    setge al\n", out);
                    else if(is_le) fputs("    setle al\n", out);
                    else fputs("    jmp L_ERR_UNKNOWN_EMBED\n", out);
                    fputs("    movzx rax, al\n", out);
                }
                fputs("    mov qword ptr [rbx + rcx*8], rax\n", out);
            }
        } break;
        case IR_CALL_MACRO:
            fputs("    jmp L_ERR_UNRESOLVED_MACRO\n", out);
            break;
    }
}

static inline int emit_ir_asm_program(const char *output_asm_path, IRProgram *ir)
{
    FILE *out = fopen(output_asm_path, "wb");
    if(!out)
    {
        perror("Failed to create asm file");
        return 0;
    }

    fputs(".intel_syntax noprefix\n", out);
    fputs(".extern printf\n", out);
    fputs(".extern scanf\n", out);
    fputs(".extern malloc\n", out);
    fputs(".extern free\n", out);
    fputs(".extern rand\n", out);
    fputs(".extern strcpy\n", out);
    fputs(".extern exit\n\n", out);

    fputs(".section .rdata,\"dr\"\n", out);
    fputs("FMT_INT_IN: .asciz \"%lld\"\n", out);
    fputs("FMT_STR_IN: .asciz \"%255s\"\n", out);
    fputs("FMT_CHR_IN: .asciz \"%c\"\n", out);
    fputs("FMT_INT_OUT: .asciz \"%lld\"\n", out);
    fputs("FMT_STR_OUT: .asciz \"%s\"\n", out);
    fputs("FMT_CHR_OUT: .asciz \"%c\"\n", out);
    fputs("FMT_ERR: .asciz \"Runtime error: %s\\n\"\n", out);
    fputs("ERR_PTR: .asciz \"pointer out of tape bounds\"\n", out);
    fputs("ERR_STACK_UNDER: .asciz \"stack underflow\"\n", out);
    fputs("ERR_STACK_OVER: .asciz \"stack overflow\"\n", out);
    fputs("ERR_TUPLE_OVER: .asciz \"tuple nesting too deep\"\n", out);
    fputs("ERR_TUPLE_UNDER: .asciz \"tuple stack underflow\"\n", out);
    fputs("ERR_MALLOC: .asciz \"malloc failed\"\n", out);
    fputs("ERR_DIV_ZERO: .asciz \"division by zero\"\n", out);
    fputs("ERR_MEM_SIZE: .asciz \"mem size must be positive\"\n", out);
    fputs("ERR_REGION: .asciz \"region ops not yet supported in asm backend\"\n", out);
    fputs("ERR_UNRES_MACRO: .asciz \"unresolved macro in asm backend\"\n", out);
    fputs("ERR_UNKNOWN_EMBED: .asciz \"unknown embedded function in asm backend\"\n", out);

    int str_id = 0;
    IRInst *inst = ir->head;
    while(inst)
    {
        if(inst->op == IR_NEW_STRING)
        {
            fprintf(out, "STR_%d: .asciz ", str_id);
            write_asm_string_literal(out, inst->text ? inst->text : "");
            fputc('\n', out);
            str_id++;
        }
        inst = inst->next;
    }

    fputs("\n.section .bss\n", out);
    fputs("    .align 8\n", out);
    fputs("tape: .zero 8192\n", out);
    fputs("tap_stack: .zero 8192\n", out);
    fputs("pointer: .zero 8\n", out);
    fputs("tap_sp: .zero 8\n", out);
    fputs("tuple_stack: .zero 1024\n", out);
    fputs("tuple_sp: .zero 8\n\n", out);

    fputs(".text\n", out);
    fputs(".globl main\n", out);
    fputs("main:\n", out);
    fputs("    push rbp\n", out);
    fputs("    mov rbp, rsp\n", out);
    fputs("    sub rsp, 40\n", out);
    fputs("    mov qword ptr [rip + pointer], 0\n", out);
    fputs("    mov qword ptr [rip + tap_sp], 0\n", out);
    fputs("    mov qword ptr [rip + tuple_sp], 0\n", out);

    int string_index = 0;
    inst = ir->head;
    while(inst)
    {
        int label = ir_label_for_token(ir, inst->origin, inst->scope);
        if(label > 0)
        {
            fprintf(out, "L%d:\n", label);
        }
        emit_asm_instruction(out, inst, string_index);
        if(inst->op == IR_NEW_STRING)
        {
            string_index++;
        }
        inst = inst->next;
    }

    fputs("L_END:\n", out);
    fputs("    xor eax, eax\n", out);
    fputs("    add rsp, 40\n", out);
    fputs("    pop rbp\n", out);
    fputs("    ret\n\n", out);

    fputs("L_ERR_PTR:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_PTR]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_STACK_UNDER:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_STACK_UNDER]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_STACK_OVER:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_STACK_OVER]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_TUPLE_OVER:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_TUPLE_OVER]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_TUPLE_UNDER:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_TUPLE_UNDER]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_MALLOC:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_MALLOC]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_DIV_ZERO:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_DIV_ZERO]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_MEM_SIZE:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_MEM_SIZE]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_REGION_UNSUPPORTED:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_REGION]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_UNRESOLVED_MACRO:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_UNRES_MACRO]\n    call printf\n    mov ecx, 1\n    call exit\n", out);
    fputs("L_ERR_UNKNOWN_EMBED:\n    lea rcx, [rip + FMT_ERR]\n    lea rdx, [rip + ERR_UNKNOWN_EMBED]\n    call printf\n    mov ecx, 1\n    call exit\n", out);

    fclose(out);
    return 1;
}

static inline int compile_to_asm_source(const char *input_path, const char *output_asm_path)
{
    char *program = 0;
    IRProgram *ir = 0;
    if(prepare_ir_from_input(input_path, &program, &ir) != 0)
    {
        return 1;
    }

    IRInst *region_inst = 0;
    if(ir_contains_region_ops(ir, &region_inst))
    {
        printf(
            "Build error at %s:%d:%d: region ops are not supported yet in asm backend\n",
            region_inst->source[0] ? region_inst->source : "<unknown>",
            region_inst->line,
            region_inst->column
        );
        free(program);
        return 1;
    }

    IRInst *unknown_embed = 0;
    if(ir_contains_unknown_embed(ir, &unknown_embed))
    {
        printf(
            "Build error at %s:%d:%d: unknown embedded function '%s' in asm backend\n",
            unknown_embed->source[0] ? unknown_embed->source : "<unknown>",
            unknown_embed->line,
            unknown_embed->column,
            unknown_embed->name
        );
        free(program);
        return 1;
    }

    if(!emit_ir_asm_program(output_asm_path, ir))
    {
        free(program);
        return 1;
    }

    free(program);
    return 0;
}

static inline int compile_asm_to_executable(const char *input_path, const char *output_path)
{
    const char *generated_asm = "__tapium_build_tmp.s";
    int asm_code = compile_to_asm_source(input_path, generated_asm);
    if(asm_code != 0)
    {
        return asm_code;
    }

    char command[1024];
    snprintf(command, sizeof(command), "gcc \"%s\" -o \"%s\"", generated_asm, output_path);
    int compile_code = system(command);
    remove(generated_asm);

    if(compile_code != 0)
    {
        printf("Build failed while assembling generated source.\n");
        return 1;
    }
    return 0;
}

static inline int compile_to_executable(const char *input_path, const char *output_path)
{
    const char *generated_c = "__tapium_build_tmp.c";
    int c_codegen_result = compile_to_c_source(input_path, generated_c);
    if(c_codegen_result != 0)
    {
        return c_codegen_result;
    }

    char command[1024];
    snprintf(command, sizeof(command), "gcc \"%s\" -o \"%s\"", generated_c, output_path);
    int compile_code = system(command);

    remove(generated_c);

    if (compile_code != 0) {
        printf("Build failed while compiling generated C source.\n");
        return 1;
    }
    return 0;
}

#endif
