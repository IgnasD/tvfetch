#include <stdio.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pcre.h>

#include "common.h"
#include "settings.h"

static struct show_struct* get_show(xmlNodePtr node) {
    xmlNodePtr xml_node = get_node_by_name(node, "regex");
    if (!xml_node) {
        fprintf(stderr, "missing /tvfetch/shows/show/regex\n");
        return NULL;
    }
    xmlChar *string_xml = xmlNodeGetContent(xml_node->children);
    const char *error_pcre;
    int erroffset_pcre;
    pcre *regex_pcre = pcre_compile((char *)string_xml, PCRE_CASELESS, &error_pcre, &erroffset_pcre, NULL);
    xmlFree(string_xml);
    if (!regex_pcre) {
        fprintf(stderr, "pcre_compile failed %s\n", error_pcre);
        return NULL;
    }
    pcre_extra *regex_pcre_extra = pcre_study(regex_pcre, 0, &error_pcre);
    if(error_pcre) {
        fprintf(stderr, "pcre_study failed %s\n", error_pcre);
        return NULL;
    }
    
    xml_node = get_node_by_name(node, "season");
    if (!xml_node) {
        fprintf(stderr, "missing /tvfetch/shows/show/season\n");
        return NULL;
    }
    xmlNodePtr season_node = xml_node->children;
    string_xml = xmlNodeGetContent(season_node);
    int season = (int) strtol((char *)string_xml, NULL, 10);
    xmlFree(string_xml);
    
    xml_node = get_node_by_name(node, "episode");
    if (!xml_node) {
        fprintf(stderr, "missing /tvfetch/shows/show/episode\n");
        return NULL;
    }
    xmlNodePtr episode_node = xml_node->children;
    string_xml = xmlNodeGetContent(episode_node);
    int episode = (int) strtol((char *)string_xml, NULL, 10);
    xmlFree(string_xml);
    
    struct show_struct *show = malloc(sizeof(struct show_struct));
    show->regex_pcre = regex_pcre;
    show->regex_pcre_extra = regex_pcre_extra;
    show->season = season;
    show->season_node = season_node;
    show->episode = episode;
    show->episode_node = episode_node;
    show->next = NULL;
    return show;
}

int get_settings(const char *filename, struct settings_struct *settings) {
    settings->filename = filename;
    settings->xml_doc = NULL;
    settings->new_shows = 0;
    settings->feed_xml = NULL;
    settings->feed = NULL;
    settings->downloaddir_xml = NULL;
    settings->downloaddir = NULL;
    settings->shows = NULL;
    
    xmlDocPtr xml_doc = xmlParseFile(filename);
    if (!xml_doc) {
        return 0;
    }
    settings->xml_doc = xml_doc;
    
    xmlNodePtr xml_node_main = xmlDocGetRootElement(xml_doc);
    if (xmlStrcmp(xml_node_main->name, (const xmlChar *)"tvfetch")) {
        fprintf(stderr, "missing /tvfetch\n");
        return 0;
    }
    xml_node_main = xml_node_main->children;
    
    xmlNodePtr xml_node = get_node_by_name(xml_node_main, "feed");
    if (!xml_node) {
        fprintf(stderr, "missing /tvfetch/feed\n");
        return 0;
    }
    settings->feed_xml = xmlNodeGetContent(xml_node->children);
    settings->feed = (char *) settings->feed_xml;
    
    xml_node = get_node_by_name(xml_node_main, "downloaddir");
    if (!xml_node) {
        fprintf(stderr, "missing /tvfetch/downloaddir\n");
        return 0;
    }
    settings->downloaddir_xml = xmlNodeGetContent(xml_node->children);
    settings->downloaddir = (char *) settings->downloaddir_xml;
    
    xml_node = get_node_by_name(xml_node_main, "shows");
    if (!xml_node) {
        fprintf(stderr, "missing /tvfetch/shows\n");
        return 0;
    }
    
    xml_node = xml_node->children;
    struct show_struct **show_ptr = &(settings->shows);
    struct show_struct *show;
    
    while (xml_node) {
        xml_node = get_node_by_name(xml_node, "show");
        if (xml_node) {
            show = get_show(xml_node->children);
            if (!show) {
                return 0;
            }
            *show_ptr = show;
            show_ptr = &(show->next);
            xml_node = xml_node->next;
        }
    }
    
    return 1;
}

void free_settings(struct settings_struct *settings) {
    if (settings->feed_xml) {
        xmlFree(settings->feed_xml);
    }
    if (settings->downloaddir_xml) {
        xmlFree(settings->downloaddir_xml);
    }
    if (settings->xml_doc) {
        if (settings->new_shows) {
            xmlSaveFile(settings->filename, settings->xml_doc);
        }
        xmlFreeDoc(settings->xml_doc);
    }
    struct show_struct *show, *temp_show;
    for (show = settings->shows; show;) {
        if (show->regex_pcre) {
            pcre_free(show->regex_pcre);
        }
        if (show->regex_pcre_extra) {
            pcre_free(show->regex_pcre_extra);
        }
        temp_show = show;
        show = show->next;
        free(temp_show);
    }
}
