#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pcre.h>

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

struct data_struct {
    char *contents;
    size_t length;
};

static xmlNodePtr get_node_by_name(xmlNodePtr node, const char *name) {
    xmlNodePtr cur_node;
    const xmlChar *xml_name = (const xmlChar *)name;

    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if (!xmlStrcmp(cur_node->name, xml_name)) {
                return cur_node;
            }
        }
    }
    
    return NULL;
}

static struct show_struct* get_settings_show(xmlNodePtr node) {
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

static int get_settings(const char *filename, struct settings_struct *settings) {
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
        //fprintf(stderr, "Missing or malformed settings file\n");
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
            show = get_settings_show(xml_node->children);
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

static void free_settings(struct settings_struct *settings) {
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

static size_t write_memory_curl_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    struct data_struct *data = (struct data_struct *)userdata;
    
    void *block = realloc(data->contents, data->length + realsize + 1);
    if(!block) {
        fprintf(stderr, "not enough memory (realloc returned NULL)\n");
        return 0;
    }
    data->contents = block;
    
    memcpy(&(data->contents[data->length]), ptr, realsize);
    data->length += realsize;
    data->contents[data->length] = 0;
    
    return realsize;
}

static int get_feed(const char *url, struct data_struct *data) {
    CURL *curl_handle;
    CURLcode curl_result;
    
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_curl_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)data);
    
    curl_result = curl_easy_perform(curl_handle);
    
    curl_easy_cleanup(curl_handle);
    
    if(curl_result != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(curl_result));
        if (data->contents) {
            free(data->contents);
        }
        data->length = 0;
        return 0;
    }
    else {
        return 1;
    }
}

static size_t write_file_curl_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    int written = fwrite(ptr, size, nmemb, (FILE *)userdata);
    return written;
}

static int download(const char *url, struct settings_struct *settings) {
    FILE *file_desc;
    CURL *curl_handle;
    CURLcode curl_result;
    char fullpath[200];
    
    snprintf(fullpath, 200, "%s%i%i.torrent", settings->downloaddir, settings->new_shows, (int)time(NULL));
    
    file_desc = fopen(fullpath, "wb");
    if (!file_desc) {
        fprintf(stderr, "error while preparing file for download\n");
        return 0;
    }
    
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file_curl_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file_desc);
    
    curl_result = curl_easy_perform(curl_handle);
    
    curl_easy_cleanup(curl_handle);
    
    fclose(file_desc);
    
    if(curl_result != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(curl_result));
        return 0;
    }
    else {
        return 1;
    }
}

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
                
                download((char *)link_string, settings);
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
        printf("Please provide config.xml as an only argument\n");
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
                    fprintf(stderr, "Malformed rss feed\n");
                }
            }
        }
    }
    
    free_settings(&settings);
    
    xmlCleanupParser();
    curl_global_cleanup();
    
    return 0;
}
