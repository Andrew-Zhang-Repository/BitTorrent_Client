#include <iostream>
#include <string>
#include <curl/curl.h>
#include <stdexcept>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::string get_response(std::string url);