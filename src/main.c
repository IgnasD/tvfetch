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

static int parse_item(xmlNodePtr node, struct settings_struct *settings) {
    xmlNodePtr title_node = get_node_by_name(node, "title");
    if (!title_node) {
        logging_error("missing /rss/channel/item/title");
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
            
            if (season > show->season || (season == show->season && episode > show->episode)) {
                show->season = season;
                snprintf(numbuf, 5, "%d", season);
                xmlNodeSetContent(show->season_node, (xmlChar *)numbuf);
                
                show->episode = episode;
                snprintf(numbuf, 5, "%d", episode);
                xmlNodeSetContent(show->episode_node, (xmlChar *)numbuf);
                
                settings->new_shows++;
                
                link_node = get_node_by_name(node, "link");
                if (!link_node) {
                    logging_error("missing /rss/channel/item/link");
                    xmlFree(title_string_xml);
                    return 0;
                }
                link_string_xml = xmlNodeGetContent(link_node->children);
                
                logging_info("Fetching \"%s\"", title_string);
                download_torrent((char *)link_string_xml, settings->downloaddir, title_string);
                
                xmlFree(link_string_xml);
            }
            
        }
    }
    
    xmlFree(title_string_xml);
    
    return 1;
}

static void parse_feed(xmlDocPtr xml_doc, struct settings_struct *settings) {
    xmlNodePtr xml_node;
    
    xml_node = xmlDocGetRootElement(xml_doc);
    if (xmlStrcmp(xml_node->name, (const xmlChar *)"rss")) {
        logging_error("missing /rss");
        return;
    }
    xml_node = get_node_by_name(xml_node->children, "channel");
    if (!xml_node) {
        logging_error("missing /rss/channel");
        return;
    }

    xml_node = xml_node->children;
    while (xml_node) {
        xml_node = get_node_by_name(xml_node, "item");
        if (xml_node) {
            if (!parse_item(xml_node->children, settings)) {
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
    
    struct data_struct feed;
    feed.contents = malloc(200000);
    if (!feed.contents) {
        logging_error("not enough memory (malloc returned NULL)");
        return 1;
    }
    feed.allocated = 200000;
    feed.length = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    xmlInitParser();
    
    if (get_settings(argv[1], &settings)) {
        if (get_feed(settings.feed, &feed)) {
            if (feed.contents) {
                xmlDocPtr xml_doc = xmlReadMemory(feed.contents, feed.length,
                        "feed.xml", NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
                free(feed.contents);
                if (xml_doc) {
                    parse_feed(xml_doc, &settings);
                    xmlFreeDoc(xml_doc);
                }
                else {
                    logging_error("malformed RSS feed");
                }
            }
        }
    }
    
    free_settings(&settings);
    
    xmlCleanupParser();
    curl_global_cleanup();
    
    return 0;
}
