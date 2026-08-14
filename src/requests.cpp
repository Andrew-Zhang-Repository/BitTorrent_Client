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

std::string recv_exact(int n,int sock){
    std::string peer_response;
    peer_response.resize(n);
    char* buffer_ptr = &peer_response[0];
    int bytes_received = 0;
    
    while (bytes_received < n) {
        int result = recv(sock, buffer_ptr + bytes_received, n - bytes_received, 0);
        if (result <= 0) {
            std::cerr << "Peer dropped connection or network error." << std::endl;
            close(sock);
            return {};
        }
        bytes_received += result;
    }
   
    return peer_response;
}

int read_message(int sock,int &ben,std::string &payload){
    std::string first_four = recv_exact(4,sock);
    std::string fith = recv_exact(1,sock);

    if (first_four.size() != 4){
        throw std::runtime_error("Did not get 4 big edian bytes after handshake");
    }

    if (fith.size() != 1){
        throw std::runtime_error("Could not get message id");
    }
    int id = static_cast<int>(static_cast<char>(fith[fith.size()-1]));

    uint32_t length = (uint8_t)first_four[0] << 24 | (uint8_t)first_four[1] << 16 |
                      (uint8_t)first_four[2] << 8  | (uint8_t)first_four[3];

    if (length == 0){return - 1;}
    payload = recv_exact(length - 1,sock);
    if (payload.size() != length - 1){ return - 1;}

    ben = length;
    

    return id;
}

void handle_message(message_code id, const std::string& payload) {
    switch (id) {
        case message_code::CHOKE:
            // choke_handler();
            break;
        case message_code::UNCHOKE:
            // unchoke_handler();
            break;
        case message_code::HAVE:
            // have_handler(payload);
            break;
        case message_code::BITFIELD:
            // bitfield_handler(payload);
            break;
        case message_code::PIECE:
            // piece_handler(payload);
            break;
        default:
            std::cout << "Ignored unhandled message ID." << std::endl;
            break;
    }
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

bool connect_and_send(std::string handshake, peer peer, std::string info_hash){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error\n";
        return false;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(peer.port);

    struct timeval tv;
    tv.tv_sec = 15;       
    tv.tv_usec = 0;     

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting timeout" << std::endl;
        return false;
    }

    if (inet_pton(AF_INET, peer.ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        close(sock);
        return false;
    }

    if (!(connectWithTimeout(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr),10))) {
        std::cerr << "Connection failed\n";
        close(sock);
        return false;
    }

    if (send(sock, handshake.c_str(), handshake.length(), 0) != 68) {
        std::cerr << "Failed to send full handshake." << std::endl;
        close(sock);
        return false;
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
        // Start recieving packets
        std::cout<< "ready to start download" << std::endl;
        int ben = 0;
        std::string payload;
        int id = read_message(sock,ben,payload);
        std::cout<< ben << std::endl;
        std::cout<< id << std::endl;
        std::cout<< payload << std::endl;
        return true;
    }
    else{
        std::cout<< "hashes don't match" << std::endl;
        close(sock);
        return false;
    }

}