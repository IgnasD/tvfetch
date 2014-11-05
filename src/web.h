#ifndef REMOTE_H
#define REMOTE_H

#ifdef __cplusplus
extern "C" {
#endif

struct data_struct {
    char *contents;
    int allocated;
    int length;
};

int get_feed(const char *url, struct data_struct *data);
int download_torrent(const char *url, const char *downloaddir, const char *title);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_H */
