#include "../include/requests.h"
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>



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

std::string get_handshake(std::string info_hash, std::string peer_id){

    std::string return_str = "";

    //insert first byte
    return_str += static_cast<char>(0x13);

    //insert next 19
    return_str += "BitTorrent protocol";

    //insert next 8
    for (int i = 0; i < 8; i++){
        return_str += static_cast<char>(0x00);
    }

    //Info hash and then peeeer
    return_str += info_hash;
    return_str += peer_id;

    return return_str;
}



//Fully generated check for issues later
bool connectWithTimeout(int sock, const struct sockaddr* addr, socklen_t addrLen, int timeoutSec) {
    // 1. Set socket to non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return false;
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) return false;

    int res = connect(sock, addr, addrLen);
    if (res < 0) {
        if (errno != EINPROGRESS) {
            return false; 
        }

        fd_set writeFds;
        FD_ZERO(&writeFds);
        FD_SET(sock, &writeFds);

        struct timeval tv;
        tv.tv_sec = timeoutSec;
        tv.tv_usec = 0;
        res = select(sock + 1, nullptr, &writeFds, nullptr, &tv);

        if (res < 0) {
            return false;
        } else if (res == 0) {
            errno = ETIMEDOUT;
            return false;
        } else {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
                return false;
            }
            if (so_error != 0) {
                errno = so_error;
                return false; 
            }
        }
    }

    if (fcntl(sock, F_SETFL, flags) < 0) return false;

    return true;
}

int connect_and_send(std::string handshake, peer peer, std::string info_hash){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(peer.port);

    struct timeval tv;
    tv.tv_sec = 15;       
    tv.tv_usec = 0;     

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting timeout" << std::endl;
        return -1;
    }

    if (inet_pton(AF_INET, peer.ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        close(sock);
        return -1;
    }

    if (!(connectWithTimeout(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr),5))) {
        std::cerr << "Connection failed\n";
        close(sock);
        return -1;
    }

    if (send(sock, handshake.c_str(), handshake.length(), 0) != 68) {
        std::cerr << "Failed to send full handshake." << std::endl;
        close(sock);
        return -1;
    }

    std::string peer_response = recv_exact(68,sock);
    /*int bytes_received = 0;
    char peer_response[68];
    while (bytes_received < 68) {
        int result = recv(sock, peer_response + bytes_received, 68 - bytes_received, 0);
        if (result <= 0) {
            std::cerr << "Peer dropped connection or network error." << std::endl;
            close(sock);
            return false;
        }
        bytes_received += result;
    }

    std::string sub(peer_response + 28, 20);
    std::string rec_hash = sub;
    */
    if (peer_response.size()!=68){
        return false;
    }
    std::string rec_hash = peer_response.substr(28,20);
    if (rec_hash == info_hash){
        std::cout<< "ready to start download" << std::endl;
        return sock;
    }
    else{
        std::cout<< "hashes don't match" << std::endl;
        close(sock);
        return -1;
    }

}