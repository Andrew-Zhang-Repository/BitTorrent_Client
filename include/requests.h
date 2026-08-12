#include <iostream>
#include <string>
#include <curl/curl.h>
#include <stdexcept>
#include "../include/reader.h"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::string get_response(std::string url);
BencodeDict extractTrackerDictionary(const std::string& raw_http_response, BencodeParser& parser);
void get_interval_peers(BencodeDict dict, BencodeInt& interval, BencodeString& peers);