#include <stdlib.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pcre.h>

#include "common.h"
#include "session.h"
#include "logging.h"

static struct feed_struct* get_feed(xmlNodePtr node) {
    // /tvfetch/feeds/feed/name
    xmlNodePtr xml_node = get_node_by_name(node, "name");
    if (!xml_node) {
        logging_error("[Session] Missing /tvfetch/feeds/feed/name");
        return NULL;
    }
    xmlChar *name_xml = xmlNodeGetContent(xml_node->children);
    if (!name_xml) {
        logging_error("[Session] Empty /tvfetch/feeds/feed/name");
        return NULL;
    }
    // ---
    
    // /tvfetch/feeds/feed/url
    xml_node = get_node_by_name(node, "url");
    if (!xml_node) {
        xmlFree(name_xml);
        logging_error("[Session] Missing /tvfetch/feeds/feed/url");
        return NULL;
    }
    xmlChar *url_xml = xmlNodeGetContent(xml_node->children);
    if (!url_xml) {
        xmlFree(name_xml);
        logging_error("[Session] Empty /tvfetch/feeds/feed/url");
        return NULL;
    }
    // ---
    
    // /tvfetch/feeds/feed/delay
    xml_node = get_node_by_name(node, "delay");
    if (!xml_node) {
        xmlFree(name_xml);
        xmlFree(url_xml);
        logging_error("[Session] Missing /tvfetch/feeds/feed/delay");
        return NULL;
    }
    xmlChar *delay_xml = xmlNodeGetContent(xml_node->children);
    if (!delay_xml) {
        xmlFree(name_xml);
        xmlFree(url_xml);
        logging_error("[Session] Empty /tvfetch/feeds/feed/delay");
        return NULL;
    }
    time_t delay = (time_t) strtol((char *)delay_xml, NULL, 10);
    xmlFree(delay_xml);
    if (delay < 0) {
        xmlFree(name_xml);
        xmlFree(url_xml);
        logging_error("[Session] /tvfetch/feeds/feed/delay must be >= 0");
        return NULL;
    }
    // ---
    
    struct feed_struct *feed = malloc(sizeof(struct feed_struct));
    if (feed) {
        feed->name_xml = name_xml;
        feed->name = (char *)name_xml;
        feed->url_xml = url_xml;
        feed->url = (char *)url_xml;
        feed->delay = delay;
        feed->next = NULL;
    }
    else {
        logging_error("[Session] Not enough memory to allocate feed struct (malloc returned NULL)");
    }
    return feed;
}

static struct show_struct* get_show(xmlNodePtr node) {
    // /tvfetch/shows/show/regex
    xmlNodePtr xml_node = get_node_by_name(node, "regex");
    if (!xml_node) {
        logging_error("[Session] Missing /tvfetch/shows/show/regex");
        return NULL;
    }
    xmlChar *string_xml = xmlNodeGetContent(xml_node->children);
    if (!string_xml) {
        logging_error("[Session] Empty /tvfetch/shows/show/regex");
        return NULL;
    }
    
    const char *error_pcre;
    int erroffset_pcre;
    
    pcre *regex_pcre = pcre_compile((char *)string_xml, PCRE_CASELESS, &error_pcre, &erroffset_pcre, NULL);
    if (!regex_pcre) {
        logging_error("[Session] pcre_compile of %s failed: %s", string_xml, error_pcre);
        xmlFree(string_xml);
        return NULL;
    }
    
    pcre_extra *regex_pcre_extra = pcre_study(regex_pcre, 0, &error_pcre);
    if (!regex_pcre_extra && error_pcre) {
        logging_error("[Session] pcre_study of %s failed: %s", string_xml, error_pcre);
        xmlFree(string_xml);
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        return NULL;
    }
    
    xmlFree(string_xml);
    // ---
    
    // /tvfetch/shows/show/season
    xml_node = get_node_by_name(node, "season");
    if (!xml_node) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] Missing /tvfetch/shows/show/season");
        return NULL;
    }
    xmlNodePtr season_node = xml_node->children;
    string_xml = xmlNodeGetContent(season_node);
    if (!string_xml) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] Empty /tvfetch/shows/show/season");
        return NULL;
    }
    int season = (int) strtol((char *)string_xml, NULL, 10);
    xmlFree(string_xml);
    if (season < 0) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] /tvfetch/shows/show/season must be >= 0");
        return NULL;
    }
    // ---
    
    // /tvfetch/shows/show/episode
    xml_node = get_node_by_name(node, "episode");
    if (!xml_node) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] Missing /tvfetch/shows/show/episode");
        return NULL;
    }
    xmlNodePtr episode_node = xml_node->children;
    string_xml = xmlNodeGetContent(episode_node);
    if (!string_xml) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] Empty /tvfetch/shows/show/episode");
        return NULL;
    }
    int episode = (int) strtol((char *)string_xml, NULL, 10);
    xmlFree(string_xml);
    if (episode < 0) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] /tvfetch/shows/show/episode must be >= 0");
        return NULL;
    }
    // ---
    
    // /tvfetch/shows/show/seen
    xml_node = get_node_by_name(node, "seen");
    if (!xml_node) {
        logging_error("[Session] Missing /tvfetch/shows/show/seen");
        return NULL;
    }
    xmlNodePtr seen_node = xml_node->children;
    string_xml = xmlNodeGetContent(seen_node);
    if (!string_xml) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] Empty /tvfetch/shows/show/seen");
        return NULL;
    }
    time_t seen = (time_t) strtol((char *)string_xml, NULL, 10);
    xmlFree(string_xml);
    if (seen < 0) {
        pcre_free(regex_pcre);
        pcre_free(regex_pcre_extra);
        logging_error("[Session] /tvfetch/shows/show/seen must be >= 0");
        return NULL;
    }
    // ---
    
    struct show_struct *show = malloc(sizeof(struct show_struct));
    if (show) {
        show->regex_pcre = regex_pcre;
        show->regex_pcre_extra = regex_pcre_extra;
        show->season = season;
        show->season_node = season_node;
        show->episode = episode;
        show->episode_node = episode_node;
        show->seen = seen;
        show->seen_node = seen_node;
        show->next = NULL;
    }
    else {
        logging_error("[Session] Not enough memory to allocate show struct (malloc returned NULL)");
    }
    return show;
}

int session_load(const char *filename, struct session_struct *session) {
    session->filename = filename;
    session->xml_doc = NULL;
    session->modified = 0;
    session->target_xml = NULL;
    session->target = NULL;
    session->feeds = NULL;
    session->shows = NULL;
    
    xmlDocPtr xml_doc = xmlParseFile(filename);
    if (!xml_doc) {
        logging_error("[Session] Error while parsing %s", filename);
        return 0;
    }
    session->xml_doc = xml_doc;
    
    xmlNodePtr xml_node_main = xmlDocGetRootElement(xml_doc);
    if (xmlStrcmp(xml_node_main->name, (const xmlChar *)"tvfetch")) {
        logging_error("[Session] Missing /tvfetch");
        return 0;
    }
    xml_node_main = xml_node_main->children;
    
    // /tvfetch/target
    xmlNodePtr xml_node = get_node_by_name(xml_node_main, "target");
    if (!xml_node) {
        logging_error("[Session] Missing /tvfetch/target");
        return 0;
    }
    session->target_xml = xmlNodeGetContent(xml_node->children);
    if (!session->target_xml) {
        logging_error("[Session] Empty /tvfetch/target");
        return 0;
    }
    session->target = (char *) session->target_xml;
    // ---
    
    xml_node = get_node_by_name(xml_node_main, "feeds");
    if (!xml_node) {
        logging_error("[Session] Missing /tvfetch/feeds");
        return 0;
    }
    
    int test;
    
    // /tvfetch/feeds/feed
    xml_node = xml_node->children;
    struct feed_struct **feed_ptr = &session->feeds;
    struct feed_struct *feed;
    
    for (test = 0; (xml_node = get_node_by_name(xml_node, "feed")); xml_node = xml_node->next) {
        feed = get_feed(xml_node->children);
        if (!feed) {
            return 0;
        }
        *feed_ptr = feed;
        feed_ptr = &feed->next;
        test = 1;
    }
    
    if (!test) {
        logging_error("[Session] Missing /tvfetch/feeds/feed");
        return 0;
    }
    // ---
    
    xml_node = get_node_by_name(xml_node_main, "shows");
    if (!xml_node) {
        logging_error("[Session] Missing /tvfetch/shows");
        return 0;
    }
    
    // /tvfetch/shows/shows
    xml_node = xml_node->children;
    struct show_struct **show_ptr = &session->shows;
    struct show_struct *show;
    
    for (test = 0; (xml_node = get_node_by_name(xml_node, "show")); xml_node = xml_node->next) {
        show = get_show(xml_node->children);
        if (!show) {
            return 0;
        }
        *show_ptr = show;
        show_ptr = &show->next;
        test = 1;
    }
    
    if (!test) {
        logging_error("[Session] Missing /tvfetch/shows/shows");
        return 0;
    }
    // ---
    
    return 1;
}

void session_save(struct session_struct *session) {
    if (session->xml_doc) {
        if (session->modified) {
            xmlSaveFile(session->filename, session->xml_doc);
        }
    }
}

void session_free(struct session_struct *session) {
    if (session->xml_doc) {
        xmlFreeDoc(session->xml_doc);
    }
    
    if (session->target_xml) {
        xmlFree(session->target_xml);
    }
    
    struct feed_struct *feed, *temp_feed;
    for (feed = session->feeds; feed;) {
        if (feed->name_xml) {
            xmlFree(feed->name_xml);
        }
        if (feed->url_xml) {
            xmlFree(feed->url_xml);
        }
        temp_feed = feed;
        feed = feed->next;
        free(temp_feed);
    }
    
    struct show_struct *show, *temp_show;
    for (show = session->shows; show;) {
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
