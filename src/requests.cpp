#include "../include/requests.h"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {

    size_t totalSize = size * nmemb;
    ((std::string*)userp)->append((char*)contents, totalSize);

    return totalSize;
}

std::string get_response(const std::string url){

    std::string responseString = "";
    CURL*  curl = curl_easy_init();

    if (curl){

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK){
            std::cout << res << std::endl;
            throw std::runtime_error("Curl problem");
        }

        curl_easy_cleanup(curl);

        
    }
    else{
        throw std::runtime_error("Curl failed to initialise");
    }

    return responseString;

}