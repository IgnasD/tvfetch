#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>

#include "web.h"
#include "logging.h"

#define GROWTH_SIZE 50000

static size_t write_memory_curl_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    struct data_struct *data = (struct data_struct *)userdata;
    size_t length = size * nmemb;
    
    size_t needed = data->length + length + 1;
    while (needed > data->allocated) {
        void *block = realloc(data->contents, data->allocated+GROWTH_SIZE);
        if (!block) {
            logging_error("[WEB_MEM] Not enough memory (realloc returned NULL)");
            return 0;
        }
        data->contents = block;
        data->allocated += GROWTH_SIZE;
    }
    
    memcpy(&(data->contents[data->length]), ptr, length);
    data->length += length;
    data->contents[data->length] = 0;
    
    return length;
}

int download_to_memory(const char *url, struct data_struct *data) {
    CURL *curl_handle;
    CURLcode curl_result;
    
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_curl_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)data);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20);
    
    curl_result = curl_easy_perform(curl_handle);
    
    curl_easy_cleanup(curl_handle);
    
    if(curl_result != CURLE_OK) {
        logging_error("[WEB_MEM] curl_easy_perform() failed: %s", curl_easy_strerror(curl_result));
        return 0;
    }
    else {
        return 1;
    }
}

static size_t write_file_curl_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    return fwrite(ptr, size, nmemb, (FILE *)userdata);
}

int download_to_file(const char *url, const char *downloaddir, const char *title, const char *extension) {
    FILE *file_desc;
    CURL *curl_handle;
    CURLcode curl_result;
    
    int dir_len = strlen(downloaddir);
    int path_length = dir_len+255+1;
    char path[path_length], path_part[path_length];
    strcpy(path, downloaddir);
    
    char c;
    int i, j, len = 255-6-strlen(extension);
    for (i=0, j=0 ; j<len ; i++, j++) {
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
    path[dir_len+j] = '.';
    path[dir_len+j+1] = 0;
    strcat(path, extension);
    
    strcpy(path_part, path);
    strcat(path_part, ".part");
    
    file_desc = fopen(path_part, "wb");
    if (!file_desc) {
        logging_error("[WEB_FILE] Error while preparing file for writing");
        return 0;
    }
    
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file_curl_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file_desc);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20);
    
    curl_result = curl_easy_perform(curl_handle);
    
    curl_easy_cleanup(curl_handle);
    
    fclose(file_desc);
    
    if(curl_result != CURLE_OK) {
        logging_error("[WEB_FILE] curl_easy_perform() failed: %s", curl_easy_strerror(curl_result));
        unlink(path_part);
        return 0;
    }
    else {
        rename(path_part, path);
        return 1;
    }
}
