#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pcre.h>

#include "common.h"
#include "settings.h"
#include "web.h"

static int parse_item(xmlNodePtr node, struct settings_struct *settings) {
    xmlNodePtr title_node = get_node_by_name(node, "title");
    if (!title_node) {
        fprintf(stderr, "missing /rss/channel/item/title\n");
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
    xmlChar *link_string;
    char numbuf[5];
    time_t timestamp;
    struct tm *timestruct;
    char timebuf[30];
    
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
                    fprintf(stderr, "missing /rss/channel/item/link\n");
                    xmlFree(title_string_xml);
                    return 0;
                }
                link_string = xmlNodeGetContent(link_node->children);
                
                time(&timestamp);
                timestruct = localtime(&timestamp);
                strftime(timebuf, 30, "%Y-%m-%d %H:%M:%S %Z", timestruct);
                printf("[%s] Fetching \"%s\"\n", timebuf, title_string);
                
                download_torrent((char *)link_string, settings);
                
                xmlFree(link_string);
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
        fprintf(stderr, "missing /rss\n");
        return;
    }
    xml_node = get_node_by_name(xml_node->children, "channel");
    if (!xml_node) {
        fprintf(stderr, "missing /rss/channel\n");
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
    feed.contents = NULL;
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
                    fprintf(stderr, "Malformed RSS feed\n");
                }
            }
        }
    }
    
    free_settings(&settings);
    
    xmlCleanupParser();
    curl_global_cleanup();
    
    return 0;
}
