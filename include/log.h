#ifndef LOG_H
#define LOG_H

#define LOG_TAG(VALUE) static const char* __log_tag__ = VALUE

#ifdef ALL_DEBUG

#define LOG_PUSH(FMT) log_push(__FILE__, __FUNCTION__, __LINE__, __log_tag__, FMT)
#define LOG_PUSH_ARGS(FMT, ...) log_push(__FILE__, __FUNCTION__, __LINE__, __log_tag__, FMT, __VA_ARGS__)
void log_push(const char* file, const char* func, int line, const char* tag, const char* format, ...);

#else

#define LOG_PUSH(FMT) log_push(__log_tag__, FMT)
#define LOG_PUSH_ARGS(FMT, ...) log_push(__log_tag__, FMT, __VA_ARGS__)
void log_push(const char* tag, const char* format, ...);

#endif

#endif // LOG_H
