#ifndef LOGGING_H
#define LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

void logging_info(const char *format, ...);
void logging_error(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_H */
