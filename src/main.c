#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pcre.h>

#include "common.h"
#include "session.h"
#include "web.h"
#include "logging.h"

static time_t current_time;

static int parse_item(xmlNodePtr node, struct session_struct *session, struct feed_struct *feed) {
    xmlNodePtr title_node = get_node_by_name(node, "title");
    if (!title_node) {
        logging_error("[%s] Missing /rss/channel/item/title", feed->name);
        return 0;
    }
    xmlChar *title_string_xml = xmlNodeGetContent(title_node->children);
    char *title_string = (char *) title_string_xml;
    
    struct show_struct *show;
    int pcre_ovector[30];
    char *substring_start, *substring_end, numbuf[20];
    int season, episode;
    xmlNodePtr link_node;
    xmlChar *link_string_xml;
    
    for (show = session->shows; show; show = show->next) {
        if (current_time-show->seen < feed->delay) continue;
        
        if (pcre_exec(show->regex_pcre, show->regex_pcre_extra,
                title_string, strlen(title_string), 0, 0, pcre_ovector,
                sizeof(pcre_ovector)/sizeof(*pcre_ovector)) != 3) continue;
        
        substring_start = &title_string[pcre_ovector[2]];
        substring_end = &title_string[pcre_ovector[3]];
        season = (int) strtol(substring_start, &substring_end, 10);
        
        if (season < show->season) continue;
        if (season > show->season+1) continue;
        
        substring_start = &title_string[pcre_ovector[4]];
        substring_end = &title_string[pcre_ovector[5]];
        episode = (int) strtol(substring_start, &substring_end, 10);
        
        if (season == show->season && episode != show->episode+1) continue;
        if (season == show->season+1 && episode != 1) continue;
        
        if (feed->delay && !show->seen) {
            logging_info("%*s | Delay %5lds | %s", session->feed_name_max_len, feed->name, feed->delay, title_string);
            show->seen = current_time;
            snprintf(numbuf, sizeof(numbuf), "%ld", show->seen);
            xmlNodeSetContent(show->seen_node, (xmlChar *)numbuf);
            session->modified = 1;
            continue;
        }
        
        link_node = get_node_by_name(node, "link");
        if (!link_node) {
            logging_error("[%s] Missing /rss/channel/item/link", feed->name);
            xmlFree(title_string_xml);
            return 0;
        }
        link_string_xml = xmlNodeGetContent(link_node->children);
        
        if (download_to_file((char *)link_string_xml, session->target, title_string, "torrent")) {
            logging_info("%*s |     Fetching | %s", session->feed_name_max_len, feed->name, title_string);
            
            show->season = season;
            snprintf(numbuf, sizeof(numbuf), "%d", season);
            xmlNodeSetContent(show->season_node, (xmlChar *)numbuf);
            
            show->episode = episode;
            snprintf(numbuf, sizeof(numbuf), "%d", episode);
            xmlNodeSetContent(show->episode_node, (xmlChar *)numbuf);
            
            show->seen = 0;
            xmlNodeSetContent(show->seen_node, (xmlChar *)"0");
            
            session->modified = 1;
        }
        else {
            logging_error("[%s] Couldn't fetch \"%s\"", feed->name, title_string);
        }
        
        xmlFree(link_string_xml);
    }
    
    xmlFree(title_string_xml);
    
    return 1;
}

static void parse_rss(xmlDocPtr xml_doc, struct session_struct *session, struct feed_struct *feed) {
    xmlNodePtr xml_node;
    
    xml_node = xmlDocGetRootElement(xml_doc);
    if (xmlStrcmp(xml_node->name, (const xmlChar *)"rss")) {
        logging_error("[%s] Missing /rss", feed->name);
        return;
    }
    xml_node = get_node_by_name(xml_node->children, "channel");
    if (!xml_node) {
        logging_error("[%s] Missing /rss/channel", feed->name);
        return;
    }

    xml_node = xml_node->children;
    while (xml_node) {
        xml_node = get_node_by_name(xml_node, "item");
        if (xml_node) {
            if (!parse_item(xml_node->children, session, feed)) {
                return;
            }
            xml_node = xml_node->next;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Please provide session.xml as an only argument\n");
        return 0;
    }
    
    time(&current_time);
    
    struct session_struct session;
    if (!session_load(argv[1], &session)) {
        session_free(&session);
        return 1;
    }
    
    struct data_struct raw_data;
    raw_data.contents = malloc(INITIAL_BUFFER_SIZE);
    if (!raw_data.contents) {
        logging_error("[MAIN] Not enough memory to allocate initial buffer (malloc returned NULL)");
        return 1;
    }
    raw_data.allocated = INITIAL_BUFFER_SIZE;
    
    curl_global_init(CURL_GLOBAL_ALL);
    xmlInitParser();
    
    struct feed_struct *feed;
    for (feed = session.feeds; feed; feed = feed->next) {
        raw_data.length = 0;
        if (!download_to_memory(feed->url, &raw_data)) {
            logging_error("[%s] Can't download RSS feed", feed->name);
            continue;
        }
        
        if (raw_data.length) {
            xmlDocPtr xml_doc = xmlReadMemory(raw_data.contents, raw_data.length,
                    NULL, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
            if (xml_doc) {
                parse_rss(xml_doc, &session, feed);
                xmlFreeDoc(xml_doc);
            }
            else {
                logging_error("[%s] Malformed RSS feed", feed->name);
            }
        }
        else {
            logging_error("[%s] Empty RSS feed", feed->name);
        }
    }
    
    free(raw_data.contents);
    session_save(&session);
    session_free(&session);
    
    xmlCleanupParser();
    curl_global_cleanup();
    
    return 0;
}
