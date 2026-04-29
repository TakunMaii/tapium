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

static inline int compile_to_c_source(const char *input_path, const char *output_c_path)
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
            "Build error at %s:%d:%d: unresolved macro call '%s' (likely recursive macro, IR backend requires inlining)\n",
            first_macro_call->source[0] ? first_macro_call->source : "<unknown>",
            first_macro_call->line,
            first_macro_call->column,
            first_macro_call->name
        );
        free(program);
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
