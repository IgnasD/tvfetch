#ifndef SETTINGS_H
#define SETTINGS_H

#include <libxml/tree.h>
#include <pcre.h>

#ifdef __cplusplus
extern "C" {
#endif

struct show_struct {
    pcre *regex_pcre;
    pcre_extra *regex_pcre_extra;
    int season;
    xmlNodePtr season_node;
    int episode;
    xmlNodePtr episode_node;
    struct show_struct *next;
};

struct settings_struct {
    const char *filename;
    xmlDocPtr xml_doc;
    int new_shows;
    xmlChar *feed_xml;
    char *feed;
    xmlChar *downloaddir_xml;
    char *downloaddir;
    struct show_struct *shows;
};

int get_settings(const char *filename, struct settings_struct *settings);
void free_settings(struct settings_struct *settings);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H */
