#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void logging(FILE *stream, const char *format, va_list ap) {
    time_t timestamp;
    struct tm *timestruct;
    char timebuf[30];
    
    time(&timestamp);
    timestruct = localtime(&timestamp);
    strftime(timebuf, 30, "%Y-%m-%d %H:%M:%S %Z", timestruct);
    
    fprintf(stream, "[%s] ", timebuf);
    vfprintf(stream, format, ap);
    fprintf(stream, "\n");
}

void logging_info(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logging(stdout, format, ap);
    va_end(ap);
}

void logging_error(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    logging(stderr, format, ap);
    va_end(ap);
}
