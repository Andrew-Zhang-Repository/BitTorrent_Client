
#include "../include/message.h"
#include <arpa/inet.h> 

void handle_message(int sock, message_code id, const std::string& payload, DownloadState &ds, long long tor_length) {
    switch (id) {
        case message_code::CHOKE:
            std::cout << "<- Peer CHOKED." << std::endl;
            break;
        
        case message_code::UNCHOKE:
            std::cout << "<- Peer UNCHOKED, request data." << std::endl;
            // TODO: Call send_request() here!
            ds.current_piece_index = 0;
            ds.current_block_offset = 0;
            send_request(sock, ds.current_piece_index, ds.current_block_offset,tor_length);
            break;
        
        case message_code::BITFIELD:
            std::cout << "<- Received BITFIELD." << std::endl;
            send_interested(sock);
            break;
            
        case message_code::PIECE:
            std::cout << "<- Received a PIECE of the file!" << std::endl;
            break;
            
        default:
            std::cout << "Ignored unhandled message ID: " << static_cast<int>(id) << std::endl;
            break;
    }
}


void send_interested(int sock) {
    // Length of one and id of 2
    std::vector<uint8_t> interested_msg = {0, 0, 0, 1, 2};
    
    int sent = send(sock, interested_msg.data(), interested_msg.size(), 0);
    if (sent == 5) {
        std::cout << "-> Sent INTERESTED (ID: 2). Waiting for peer to UNCHOKE us..." << std::endl;
    } else {
        std::cerr << "Failed to send INTERESTED message." << std::endl;
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

void send_request(int sock, uint32_t piece_index, uint32_t block_offset, long long block_length) {
    std::vector<uint8_t> send_vect(17);
    send_vect.reserve(17);
    uint32_t net_len = htonl(13);
    std::memcpy(send_vect.data(), &net_len, 4);
  
    send_vect[4] = 6;

    uint32_t net_index = htonl(piece_index);
    std::memcpy(send_vect.data() + 5, &net_index, 4);

    uint32_t net_offset = htonl(block_offset);
    std::memcpy(send_vect.data() + 9, &net_offset, 4);

    uint32_t net_block_len = htonl(block_length);
    std::memcpy(send_vect.data() + 13, &net_block_len, 4);

    send(sock, send_vect.data(), send_vect.size(), 0);
    std::cout << "-> Sent REQUEST for Piece: " << piece_index << ", Offset: " << block_offset << std::endl;
}

void run_message_loop(int sock, long long tor_length) {

    struct DownloadState ds;
    while (true) { 
      
        std::string length_str = recv_exact(4, sock);
        if (length_str.size() != 4) break; 

        uint32_t length = (static_cast<uint8_t>(length_str[0]) << 24) | 
                          (static_cast<uint8_t>(length_str[1]) << 16) | 
                          (static_cast<uint8_t>(length_str[2]) << 8)  | 
                          (static_cast<uint8_t>(length_str[3]));

        if (length == 0) continue; 
        std::string id_str = recv_exact(1, sock);
        if (id_str.size() != 1) break; 

        message_code msg_id = static_cast<message_code>(static_cast<uint8_t>(id_str[0]));
        
        std::string payload = "";
        if (length > 1) {
            payload = recv_exact(length - 1, sock);
            if (payload.size() != length - 1) break;
        }
   
        //Handle the message
        handle_message(sock, msg_id, payload, ds, tor_length);
    }
}