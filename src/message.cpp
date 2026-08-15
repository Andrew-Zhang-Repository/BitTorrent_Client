
#include "../include/message.h"

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
