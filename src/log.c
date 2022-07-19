#include <include/log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* strstr_last(const char* s1, const char* s2) {
    char* old = (char*)s1;
    char* result = strstr(s1, s2);
    while(result) {
        old = result;
        result = strstr(result + 1, s2);
    }

    return old;
}


#ifdef ALL_DEBUG

void log_push(const char* file, const char* func, int line, const char* tag, const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
    printf("[%s:%d@%s] [%s] ", strstr_last(file, "/") + 1, line, func, tag);
    vprintf(format, arglist);
    printf("\n");
    va_end(arglist);
}


#else


void log_push(const char* tag, const char* format, ...) {
    va_list arglist;
    va_start(arglist, format);
    printf("[%s] ", tag);
    vprintf(format, arglist);
    printf("\n");
    va_end(arglist);
}

#endif
