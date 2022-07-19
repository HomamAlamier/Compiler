#include <include/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

char* read_file_full(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Cannot open file %s.", filename);
        return NULL;
    }

    char* buffer = NULL;
    size_t size = 0;
    while (!feof(file)) {
        char tmp[1024];
        size_t count = fread(tmp, 1, 1024, file);
        buffer = realloc(buffer, size + count);
        memcpy(&buffer[size], tmp, count);
        size += count;
    }

    fclose(file);

    buffer = realloc(buffer, size + 1);
    buffer[size] = '\0';
    return buffer;
}
