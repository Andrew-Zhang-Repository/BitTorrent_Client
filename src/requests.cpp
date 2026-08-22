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

    if (!(connectWithTimeout(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr),10))) {
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
    if (peer_response.size()!=68){
        return -1;
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

std::pair<std::string, int> extractHostAndPort(std::string_view str) {
    
    if (auto pos = str.find("://"); pos != std::string_view::npos) {
        str.remove_prefix(pos + 3);
    }

    auto colon_pos = str.rfind(':');
    if (colon_pos == std::string_view::npos) {
        return {std::string(str), 0}; 
    }

    std::string host(str.substr(0, colon_pos));
    int port = std::stoi(std::string(str.substr(colon_pos + 1)));
    
    return {host, port};
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
std::string udp_response(TorrentFile tf,std::string hashed ,std::string peer_id, std::string host, int port){

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return "";
    }

    struct timeval tv{5, 0};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
    }

    struct addrinfo hints{};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(
        host.c_str(),
        std::to_string(port).c_str(),
        &hints,
        &result
    );

    if (status!=0){
        std::cout << gai_strerror(status)  << std::endl;
        return "";
    }
  
    unsigned char connect_req[16];
    uint64_t magic_id = 0x41727101980;
    uint32_t action = 0;             
    uint32_t trans_id = 12345;        

    for (int i = 0; i < 8; i++){
        connect_req[i] = (magic_id >> (56 - 8 * i)) & 0xFF;
    }

    uint32_t action_net = htonl(action);
    uint32_t trans_net = htonl(trans_id);
    memcpy(connect_req + 8, &action_net, 4);
    memcpy(connect_req + 12, &trans_net, 4);

    sendto(sock, connect_req, 16, 0, result->ai_addr, result->ai_addrlen);
    unsigned char connect_res[16];
    int connect_rx = recvfrom(sock, connect_res, 16, 0, nullptr, nullptr);
    if (connect_rx < 16) {
        std::cerr << "UDP connect response too short: " << connect_rx << " bytes\n";
        freeaddrinfo(result);
        close(sock);
        return "";
    }
    if (connect_res[0] != 0) {
        std::cerr << "UDP connect error action: " << static_cast<int>(connect_res[0]) << "\n";
        freeaddrinfo(result);
        close(sock);
        return "";
    }
    uint32_t rx_trans_id = 0;
    memcpy(&rx_trans_id, connect_res + 4, 4);
    rx_trans_id = ntohl(rx_trans_id);
    if (rx_trans_id != trans_id) {
        std::cerr << "UDP connect transaction ID mismatch\n";
        freeaddrinfo(result);
        close(sock);
        return "";
    }

    uint64_t connection_id = 0;
    for (int i = 0; i < 8; i++){
        connection_id = (connection_id << 8) | connect_res[8 + i];
    }

    unsigned char announce_req[98];
    uint32_t announce_action = htonl(1); 
    uint32_t announce_trans = htonl(12345);

    for (int i = 0; i < 8; i++) {
        announce_req[i] = (connection_id >> (56 - 8 * i)) & 0xFF;
    }

    memcpy(announce_req + 8, &announce_action, 4);
    memcpy(announce_req + 12, &announce_trans, 4);
    memcpy(announce_req + 16, hashed.data(), 20);
    memcpy(announce_req + 36, peer_id.data(), 20);
    memset(announce_req + 56, 0, 8);

    uint64_t left = tf.length; 
    for (int i = 0; i < 8; i++) {
        announce_req[64 + i] = (left >> (56 - 8 * i)) & 0xFF;
    }

    memset(announce_req + 72, 0, 8);
    memset(announce_req + 80, 0, 4);
    memset(announce_req + 84, 0, 4);
    uint32_t key = htonl(12345);
    memcpy(announce_req + 88, &key, 4);

    uint32_t num_want = htonl(-1);
    memcpy(announce_req + 92, &num_want, 4);

    uint16_t port_net = htons(6881);
    memcpy(announce_req + 96, &port_net, 2);

    sendto(sock, announce_req, 98, 0, result->ai_addr, result->ai_addrlen);
    std::string response;
    response.resize(2048);
    char* buffer_ptr = &response[0];
    int bytes_rx = recvfrom(sock, buffer_ptr, 2048, 0, nullptr, nullptr);
    freeaddrinfo(result);
    close(sock);
    
    if (bytes_rx < 0) {
        return ""; 
    }
    
    if (bytes_rx < 8) {
        std::cerr << "UDP announce response too short: " << bytes_rx << " bytes\n";
        return "";
    }
    
    unsigned char announce_action_rx = buffer_ptr[0];
    if (announce_action_rx == 3) {
        std::string error_msg(buffer_ptr + 8, bytes_rx - 8);
        std::cerr << "UDP announce error: " << error_msg << "\n";
        return "";
    }
    
    response.resize(bytes_rx);
    
    return response;
}