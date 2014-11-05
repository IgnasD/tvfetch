#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "web.h"

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

int get_feed(const char *url, struct data_struct *data) {
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

int download_torrent(const char *url, const char *downloaddir, const char *title) {
    FILE *file_desc;
    CURL *curl_handle;
    CURLcode curl_result;
    
    int dir_len = strlen(downloaddir);
    char path[dir_len+255+1];
    strcpy(path, downloaddir);
    
    char c;
    int i, j;
    for (i=0, j=0 ; i<255-8 ; i++, j++) { // ".torrent" 8 chars
        c = title[i];
        if (c == 0) {
            break;
        }
        else if (c == '-' || c == '.' || c == '_' ||
                (c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z')) {
            path[dir_len+j] = c;
        }
        else if (path[dir_len+j-1] != '_') {
            path[dir_len+j] = '_';
        }
        else {
            j--;
        }
    }
    path[dir_len+j] = 0;
    strcat(path, ".torrent");
    
    file_desc = fopen(path, "wb");
    if (!file_desc) {
        fprintf(stderr, "error while preparing file for writing\n");
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
