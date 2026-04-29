#ifndef COMPILE_H
#define COMPILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "file.h"

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

static inline int compile_to_executable(const char *input_path, const char *output_path)
{
    char *program = load_expanded_program(input_path);
    if (!program) {
        return 1;
    }

    const char *generated_c = "__tapium_build_tmp.c";
    FILE *out = fopen(generated_c, "wb");
    if (!out) {
        perror("Failed to create generated C file");
        free(program);
        return 1;
    }

    fputs("#include <stdlib.h>\n", out);
    fputs("#include <string.h>\n", out);
    fputs("#include \"src/hashmap.h\"\n", out);
    fputs("#include \"src/simulate.h\"\n", out);
    fputs("#include \"src/token.h\"\n", out);
    fputs("#include \"src/file.h\"\n", out);
    fputs("#include \"src/embeded_func.h\"\n\n", out);
    fputs("static const char *TAP_PROGRAM = ", out);
    write_c_string_literal(out, program);
    fputs(";\n\n", out);
    fputs("int main(void)\n{\n", out);
    fputs("    char *program = (char*)malloc(strlen(TAP_PROGRAM) + 1);\n", out);
    fputs("    if (!program) {\n", out);
    fputs("        return 1;\n", out);
    fputs("    }\n", out);
    fputs("    strcpy(program, TAP_PROGRAM);\n", out);
    fputs("    macroMap = token_hashmap_create(127);\n", out);
    fputs("    if (!macroMap) {\n", out);
    fputs("        free(program);\n", out);
    fputs("        return 1;\n", out);
    fputs("    }\n", out);
    fputs("    int length = 0;\n", out);
    fputs("    Token *tokens = tokenize(program, &length, ", out);
    write_c_string_literal(out, input_path);
    fputs(");\n", out);
    fputs("    pushRegionWith(0);\n", out);
    fputs("    register_embeded_func();\n", out);
    fputs("    simulate(tokens);\n", out);
    fputs("    return 0;\n", out);
    fputs("}\n", out);
    fclose(out);

    char command[1024];
    snprintf(command, sizeof(command), "gcc \"%s\" -o \"%s\"", generated_c, output_path);
    int compile_code = system(command);

    remove(generated_c);
    free(program);

    if (compile_code != 0) {
        printf("Build failed while compiling generated C source.\n");
        return 1;
    }
    return 0;
}

#endif
