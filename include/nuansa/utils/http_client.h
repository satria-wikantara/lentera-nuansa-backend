#ifndef NUANSA_UTILS_HTTP_CLIENT_H
#define NUANSA_UTILS_HTTP_CLIENT_H

#include <string>
#include <vector>
#include <curl/curl.h>

namespace nuansa::utils {
    class HttpClient {
    public:
        struct Response {
            bool success;
            std::string body;
            std::string error;
            long statusCode;
        };

        HttpClient();
        ~HttpClient();

        Response Get(const std::string& url, 
                    const std::vector<std::string>& headers = {});
        
    private:
        CURL* curl;
        static size_t WriteCallback(void* contents, 
                                  size_t size, 
                                  size_t nmemb, 
                                  std::string* userp);
    };
}

#endif //NUANSA_UTILS_HTTP_CLIENT_H 