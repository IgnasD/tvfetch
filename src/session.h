#ifndef SESSION_H
#define SESSION_H

#include <time.h>
#include <libxml/tree.h>
#include <pcre.h>

#ifdef __cplusplus
extern "C" {
#endif

struct feed_struct {
    xmlChar *name_xml;
    char *name;
    xmlChar *url_xml;
    char *url;
    time_t delay;
    struct feed_struct *next;
};

struct show_struct {
    pcre *regex_pcre;
    pcre_extra *regex_pcre_extra;
    int season;
    xmlNodePtr season_node;
    int episode;
    xmlNodePtr episode_node;
    time_t seen;
    xmlNodePtr seen_node;
    struct show_struct *next;
};

struct session_struct {
    const char *filename;
    xmlDocPtr xml_doc;
    int modified;
    xmlChar *target_xml;
    char *target;
    struct feed_struct *feeds;
    size_t feed_name_max_len;
    struct show_struct *shows;
};

int session_load(const char *filename, struct session_struct *settings);
void session_save(struct session_struct *settings);
void session_free(struct session_struct *settings);

#ifdef __cplusplus
}
#endif

#endif /* SESSION_H */
