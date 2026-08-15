#include <iostream>
#include <string>
#include <curl/curl.h>
#include <stdexcept>
#include "../include/reader.h"
#include "../include/torrent_engine.h"
#include "../include/peer.h"
#include "../include/message.h"


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::string get_response(std::string url);
BencodeDict extractTrackerDictionary(const std::string& raw_http_response, BencodeParser& parser);
void get_interval_peers(BencodeDict dict, BencodeInt& interval, BencodeString& peers);
std::string get_handshake(std::string info_hash, std::string peer_id);
int connect_and_send(std::string handshake, peer peer, std::string info_hash);
bool connectWithTimeout(int sock, const struct sockaddr* addr, socklen_t addrLen, int timeoutSec);