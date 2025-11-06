#include <stdio.h>
#include <string>
#include <curl/curl.h>
#pragma warning(disable:4996)
struct memfile {
    char* buffer = NULL;
    size_t size = 0;
};
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    if (userp == NULL) {
        return 0;
    }
    // 将下载的内容追加到内存缓冲区
    memfile* mfile = static_cast<memfile*>(userp);
    size_t currentSize = mfile->size;
    size_t newSize = currentSize + realSize;

    // 重新分配内存以容纳更多数据
    mfile->buffer = static_cast<char*>(realloc(mfile->buffer, newSize + 1));  // +1 for null terminator
    if (mfile->buffer == nullptr) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 0;
    }

    // 将新下载的内容追加到缓冲区
    memcpy(mfile->buffer + currentSize, contents, realSize);
    (mfile->buffer)[newSize] = '\0';  // 确保缓冲区是以null结尾的字符串
    mfile->size = newSize;
    return realSize;
}

bool DownloadFileWithCurl(const std::string& url, const std::string& outputpath) {
    CURL* curl;
    CURLcode res;
    memfile mfile = { 0 };  // 用于存储下载内容

    printf("Downloading %s to %s\n", url.c_str(),outputpath.c_str());
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mfile);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Failed to download file: %s\n", curl_easy_strerror(res));
            free(mfile.buffer);  // 释放内存
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }

        // 打开文件进行写入
        if (mfile.size == 0) {
            fprintf(stderr, "Output file empty: %s\n", outputpath.c_str());
            if (mfile.buffer) free(mfile.buffer);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }
        FILE* outfile = fopen(outputpath.c_str(), "wb");
        if (!outfile) {
            fprintf(stderr, "Failed to open output file: %s\n" ,outputpath.c_str());
            free(mfile.buffer);  // 释放内存
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }
        if (mfile.buffer == nullptr) {
            curl_global_cleanup();
            return false;
        }
        // 将内存中的数据写入文件
        printf("Downloaded %s, size = %lld\n", outputpath.c_str(), mfile.size);
        fwrite(mfile.buffer, sizeof(char), mfile.size, outfile);

        // 清理资源
        fclose(outfile);
        free(mfile.buffer);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return true;
    }

    free(mfile.buffer);  // 释放内存
    curl_global_cleanup();
    return false;
}