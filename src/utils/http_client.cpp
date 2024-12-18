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
        LOG_DEBUG << "Getting URL: " << url;

        Response response{false, "", "", 0};
        std::string responseData;

        if (!curl) {
            curl = curl_easy_init();
            if (!curl) {
                LOG_ERROR << "CURL not initialized and reinit failed";
                response.error = "CURL initialization failed";
                return response;
            }
        }

        try {
            std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> 
                headerList(nullptr, curl_slist_free_all);

            curl_easy_reset(curl);
            
            // Set basic CURL options with error checking
            auto setopt = [this](CURLoption option, auto value) {
                CURLcode res = curl_easy_setopt(curl, option, value);
                if (res != CURLE_OK) {
                    throw std::runtime_error(std::string("CURL setopt failed: ") + 
                                          curl_easy_strerror(res));
                }
            };

            // Basic options
            setopt(CURLOPT_URL, url.c_str());
            setopt(CURLOPT_FOLLOWLOCATION, 1L);
            setopt(CURLOPT_NOSIGNAL, 1L);
            
            // Add timeouts
            setopt(CURLOPT_TIMEOUT, 10L);           // Total timeout in seconds
            setopt(CURLOPT_CONNECTTIMEOUT, 5L);     // Connection timeout
            setopt(CURLOPT_LOW_SPEED_TIME, 3L);     // Time in seconds for low speed
            setopt(CURLOPT_LOW_SPEED_LIMIT, 100L);  // Speed in bytes per second
            
            setopt(CURLOPT_SSL_VERIFYPEER, 1L);
            setopt(CURLOPT_SSL_VERIFYHOST, 2L);
            
            // Verbose debug output
            #ifdef DEBUG
            setopt(CURLOPT_VERBOSE, 1L);
            #endif
            
            // Callbacks
            setopt(CURLOPT_WRITEFUNCTION, WriteCallback);
            setopt(CURLOPT_WRITEDATA, &responseData);
            setopt(CURLOPT_HEADERFUNCTION, 
                +[](char* buffer, size_t size, size_t nitems, void* userdata) -> size_t {
                    if (!buffer || !userdata) return 0;
                    auto* headers = static_cast<std::vector<std::string>*>(userdata);
                    headers->emplace_back(buffer, size * nitems);
                    return size * nitems;
                });
            setopt(CURLOPT_HEADERDATA, &response.headers);

            // Set custom headers
            curl_slist* list = nullptr;
            for (const auto& header : headers) {
                list = curl_slist_append(list, header.c_str());
                if (!list) {
                    throw std::runtime_error("Failed to append header");
                }
            }
            
            if (list) {
                headerList.reset(list);
                setopt(CURLOPT_HTTPHEADER, list);
            }

            // Perform request with timeout handling
            CURLcode res;
            {
                const auto start = std::chrono::steady_clock::now();
                res = curl_easy_perform(curl);
                const auto duration = std::chrono::steady_clock::now() - start;
                LOG_DEBUG << "Request took " 
                         << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() 
                         << "ms";
            }

            if (res != CURLE_OK) {
                throw std::runtime_error(std::string("CURL request failed: ") + 
                                       curl_easy_strerror(res));
            }

            long httpCode;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            response.statusCode = httpCode;
            response.body = responseData;
            response.success = (httpCode >= 200 && httpCode < 300);

            if (!response.success) {
                LOG_ERROR << "Request failed with status " << httpCode 
                         << ": " << responseData;
            }

            return response;

        } catch (const std::exception& e) {
            LOG_ERROR << "HTTP GET failed for URL " << url << ": " << e.what();
            response.error = e.what();
            return response;
        }
    }

    HttpClient::Response HttpClient::Post(const std::string& url,
                                         const std::string& body,
                                         const std::vector<std::string>& headers,
                                         const std::string& username,
                                         const std::string& password) {
        Response response{false, "", "", 0};
        std::string responseData;

        try {
            curl_easy_reset(curl);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

            // Set Basic Auth if provided
            if (!username.empty()) {
                curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
                curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
            }

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
        } catch (const std::exception& e) {
            LOG_ERROR << "HTTP POST failed: " << e.what();
            response.error = e.what();
            return response;
        }
    }
} 