#ifndef REMOTE_H
#define REMOTE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct data_struct {
    char *contents;
    size_t allocated;
    size_t length;
};

int download_to_memory(const char *url, struct data_struct *data);
int download_to_file(const char *url, const char *downloaddir, const char *title, const char *extension);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_H */
