#ifndef REMOTE_H
#define	REMOTE_H

#include <stdlib.h>

#include "settings.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct data_struct {
    char *contents;
    size_t length;
};

int get_feed(const char *url, struct data_struct *data);
int download_torrent(const char *url, struct settings_struct *settings);

#ifdef	__cplusplus
}
#endif

#endif	/* REMOTE_H */
