#include <stdio.h>
#include <stdlib.h>
#include "simulate.h"

char* read_file(const char *filename, long *file_size) {
    FILE *file = fopen(filename, "rb");  // 二进制模式读取
    if (!file) {
        perror("无法打开文件");
        return NULL;
    }

    // 移动到文件末尾获取文件大小
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);  // 回到文件开头

    // 分配内存（+1 用于存储字符串结束符）
    char *buffer = (char*)malloc(*file_size + 1);
    if (!buffer) {
        perror("内存分配失败");
        fclose(file);
        return NULL;
    }

    // 读取整个文件
    size_t bytes_read = fread(buffer, 1, *file_size, file);
    if (bytes_read != (size_t)*file_size) {
        perror("读取文件失败");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[*file_size] = '\0';  // 添加结束符
    fclose(file);
    return buffer;
}

int main(int argn, char** argv)
{
    if(argn < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    long fileLength;
    char *fileContent = read_file(argv[1], &fileLength);

    Token *tokens = tokenize(fileContent);
    printf("===TOKENS===\n");
    printTokens(tokens);
    printf("===END TOKENS===\n");

    simulate(tokens);
    return 0;
}
