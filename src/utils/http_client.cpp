#include "nuansa/utils/http_client.h"
#include "nuansa/utils/log/log.h"

namespace nuansa::utils {
    HttpClient::HttpClient() {
        curl = curl_easy_init();
        if (!curl) {
            LOG_ERROR << "Failed to initialize CURL";
            throw std::runtime_error("Failed to initialize CURL");
        }
    }

    HttpClient::~HttpClient() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }

    size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    HttpClient::Response HttpClient::Get(const std::string& url, 
                                       const std::vector<std::string>& headers) {
        Response response{false, "", "", 0};
        std::string responseData;

        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

        // Set headers if any
        struct curl_slist* headerList = nullptr;
        for (const auto& header : headers) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        if (headerList) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        }

        CURLcode res = curl_easy_perform(curl);
        
        if (headerList) {
            curl_slist_free_all(headerList);
        }

        if (res != CURLE_OK) {
            response.error = curl_easy_strerror(res);
            return response;
        }

        long httpCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
        response.body = responseData;
        response.success = (httpCode >= 200 && httpCode < 300);

        return response;
    }
} 