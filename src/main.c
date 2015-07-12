#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pcre.h>

#include "common.h"
#include "settings.h"
#include "web.h"
#include "logging.h"

#define INITIAL_BUFFER_SIZE 300000

static int parse_item(xmlNodePtr node, struct settings_struct *settings, struct feed_struct *feed) {
    xmlNodePtr title_node = get_node_by_name(node, "title");
    if (!title_node) {
        logging_error("[%s] Missing /rss/channel/item/title", feed->name);
        return 0;
    }
    xmlChar *title_string_xml = xmlNodeGetContent(title_node->children);
    char *title_string = (char *) title_string_xml;
    
    struct show_struct *show;
    int pcre_ovector[30];
    char *substring_start;
    char *substring_end;
    int season, episode;
    xmlNodePtr link_node;
    xmlChar *link_string_xml;
    char numbuf[5];
    
    for (show = settings->shows; show; show = show->next) {
        if (pcre_exec(show->regex_pcre, show->regex_pcre_extra,
                title_string, strlen(title_string),
                0, 0, pcre_ovector, 30) == 3) {
            
            substring_start = &title_string[pcre_ovector[2]];
            substring_end = &title_string[pcre_ovector[3]];
            season = (int) strtol(substring_start, &substring_end, 10);
            
            substring_start = &title_string[pcre_ovector[4]];
            substring_end = &title_string[pcre_ovector[5]];
            episode = (int) strtol(substring_start, &substring_end, 10);
            
            if (season > show->season || (season == show->season && episode-show->episode == 1)) {
                link_node = get_node_by_name(node, "link");
                if (!link_node) {
                    logging_error("[%s] Missing /rss/channel/item/link", feed->name);
                    xmlFree(title_string_xml);
                    return 0;
                }
                link_string_xml = xmlNodeGetContent(link_node->children);
                
                logging_info("[%s] Fetching \"%s\"", feed->name, title_string);
                
                if (download_to_file((char *)link_string_xml, settings->downloaddir, title_string, "torrent")) {
                    show->season = season;
                    snprintf(numbuf, 5, "%d", season);
                    xmlNodeSetContent(show->season_node, (xmlChar *)numbuf);
                    
                    show->episode = episode;
                    snprintf(numbuf, 5, "%d", episode);
                    xmlNodeSetContent(show->episode_node, (xmlChar *)numbuf);
                    
                    settings->new_shows++;
                }
                else {
                    logging_error("[%s] Couldn't fetch \"%s\"", feed->name, title_string);
                }
                
                xmlFree(link_string_xml);
            }
            
        }
    }
    
    xmlFree(title_string_xml);
    
    return 1;
}

static void parse_rss(xmlDocPtr xml_doc, struct settings_struct *settings, struct feed_struct *feed) {
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
            if (!parse_item(xml_node->children, settings, feed)) {
                return;
            }
            xml_node = xml_node->next;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Please provide settings.xml as an only argument\n");
        return 0;
    }
    
    struct settings_struct settings;
    
    struct data_struct raw_data;
    raw_data.contents = malloc(INITIAL_BUFFER_SIZE);
    if (!raw_data.contents) {
        logging_error("[MAIN] Not enough memory to allocate initial buffer (malloc returned NULL)");
        return 1;
    }
    raw_data.allocated = INITIAL_BUFFER_SIZE;
    
    curl_global_init(CURL_GLOBAL_ALL);
    xmlInitParser();
    
    if (get_settings(argv[1], &settings)) {
        struct feed_struct *feed;
        for (feed = settings.feeds; feed; feed = feed->next) {
            raw_data.length = 0;
            if (!download_to_memory(feed->url, &raw_data)) {
                logging_error("[%s] Can't download RSS feed", feed->name);
                continue;
            }
            
            if (raw_data.length) {
                xmlDocPtr xml_doc = xmlReadMemory(raw_data.contents, raw_data.length,
                        NULL, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
                if (xml_doc) {
                    parse_rss(xml_doc, &settings, feed);
                    xmlFreeDoc(xml_doc);
                }
                else {
                    logging_error("[%s] Malformed RSS feed", feed->name);
                }
            }
        }
    }
    
    free(raw_data.contents);
    free_settings(&settings);
    
    xmlCleanupParser();
    curl_global_cleanup();
    
    return 0;
}
