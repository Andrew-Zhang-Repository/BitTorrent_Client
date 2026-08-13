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
            throw std::runtime_error("Curl problem");
        }

        curl_easy_cleanup(curl);

        
    }
    else{
        throw std::runtime_error("Curl failed to initialise");
    }

    return responseString;

}



BencodeDict extractTrackerDictionary(const std::string& raw_http_response, BencodeParser& parser) {
    
    if (raw_http_response.empty()) {
        throw std::runtime_error("Tracker response was completely empty.");
    }

    size_t index = 0;
    
    try {
    
        auto root_node = parser.decodeElement(raw_http_response, index);
        if (!std::holds_alternative<BencodeDict>(root_node->value)) {
            throw std::runtime_error("Invalid tracker response: Expected a dictionary.");
        }
        
        return std::get<BencodeDict>(root_node->value);
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse tracker response: ") + e.what());
    }
}

void get_interval_peers(BencodeDict dict, BencodeInt& interval, BencodeString& peers){

    if (dict.count("failure reason")) {
        auto reason = std::get<BencodeString>(dict["failure reason"]->value);

        std::cout << "Tracker failure: " << reason << '\n';
    }

    interval = std::get<BencodeInt>(dict["interval"]->value);
    peers = std::get<BencodeString>(dict["peers"]->value);

}