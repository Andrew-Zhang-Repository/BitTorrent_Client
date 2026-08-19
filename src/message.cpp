
#include "../include/message.h"
#include <arpa/inet.h> 
#include <map> 
// Standard BitTorrent block size is always 16 KB
const uint32_t STANDARD_BLOCK_SIZE = 16384;

uint32_t piece_size(const TorrentFile& tf, uint32_t index) {
    uint32_t total_pieces = tf.pieces.length() / 20;
    if (index == total_pieces - 1) {
        uint32_t last = tf.length % tf.piece_length;
        if (last == 0) last = tf.piece_length;
        return last;
    }
    return tf.piece_length;
}

void fill_pipeline(int sock, PeerState& ds, const TorrentFile& tf) {
    uint32_t target_size = piece_size(tf, ds.current_piece_index);
    while (ds.pending_requests.size() < ds.pipeline_depth && ds.current_block_offset < target_size) {
        uint32_t remaining = target_size - ds.current_block_offset;
        uint32_t req_size = std::min(STANDARD_BLOCK_SIZE, remaining);
        ds.pending_requests.push_back(send_request(sock, ds.current_piece_index, ds.current_block_offset, req_size));
        ds.current_block_offset += req_size;
    }
}



bool handle_message(int sock, message_code id, const std::string& payload, PeerState &ds, TorrentFile tf, std::ofstream &outfile, GlobalTorrentState& gs) {
    switch (id) {
        case message_code::CHOKE:
            std::cout << "<- Peer CHOKED." << std::endl;
            break;
        
        case message_code::UNCHOKE:{
            std::cout << "<- Peer UNCHOKED, request data." << std::endl;
            // Get work from work queue
            int piece_num = get_work(gs);

            if (piece_num == -1) {
                std::cout << "No more pieces left to download!" << std::endl;
                return true;
            }
            ds.current_piece_index = piece_num;
            ds.current_block_offset = 0;
            ds.bytes_received = 0;
            ds.piece_buffer.clear();
            ds.piece_buffer.assign(piece_size(tf, ds.current_piece_index), '\0'); 
            ds.pending_requests.clear();
            fill_pipeline(sock, ds, tf);
            
            break;
        }
        
        case message_code::BITFIELD:
            std::cout << "<- Received BITFIELD." << std::endl;
            send_interested(sock);
            break;
            
        case message_code::PIECE:

            std::cout << "<- Received a PIECE of the file!" << std::endl;
            return retrieve(ds,tf,payload,sock,outfile,gs);
            
        default:
            std::cout << "Ignored unhandled message ID: " << static_cast<int>(id) << std::endl;
            break;
    }
    return false;
}

bool retrieve(PeerState &ds, TorrentFile tf ,const std::string& payload, int sock, std::ofstream &outfile, GlobalTorrentState& gs){
    std::string block_data = payload.substr(8);

    uint32_t index = (static_cast<uint32_t>(static_cast<unsigned char>(payload[0])) << 24) |
    (static_cast<uint32_t>(static_cast<unsigned char>(payload[1])) << 16) |
    (static_cast<uint32_t>(static_cast<unsigned char>(payload[2])) << 8)  |
    static_cast<uint32_t>(static_cast<unsigned char>(payload[3]));

    uint32_t offset = (static_cast<uint32_t>(static_cast<unsigned char>(payload[4])) << 24) |
    (static_cast<uint32_t>(static_cast<unsigned char>(payload[5])) << 16) |
    (static_cast<uint32_t>(static_cast<unsigned char>(payload[6])) << 8)  |
    static_cast<uint32_t>(static_cast<unsigned char>(payload[7]));


    // Get correct request from vector
    for (auto i = ds.pending_requests.begin();i != ds.pending_requests.end(); ++i){
        if (i->block_offset == offset && i->piece_index == index){
            ds.pending_requests.erase(i); 
            break;
        }
    }


    std::copy(block_data.begin(), block_data.end(), ds.piece_buffer.begin() + offset);
    ds.bytes_received += block_data.size();

    uint32_t target_size = piece_size(tf, index);

    if (ds.bytes_received == target_size){
        std::string expected_hash = tf.pieces.substr(index * 20, 20);
        std::string actual_hash = calculateSHA1(ds.piece_buffer);

        if (actual_hash == expected_hash) {
            std::cout << "[SUCCESS] Hash match! Writing to disk" << std::endl;
            
            {
                std::lock_guard<std::mutex> lock(gs.file_mutex);
                outfile.seekp(index * tf.piece_length);
                outfile.write(ds.piece_buffer.data(), ds.piece_buffer.size());
                outfile.flush();
            }
            
            mark_piece_downloaded(gs, index);
            if (gs.done) return true;

            int next_piece = get_work(gs);
            if (next_piece == -1) return true;

            ds.current_piece_index = next_piece;
            ds.current_block_offset = 0;
            ds.bytes_received = 0;
            ds.piece_buffer.clear();
            ds.piece_buffer.assign(piece_size(tf, ds.current_piece_index), '\0');
            ds.pending_requests.clear();
            fill_pipeline(sock, ds, tf);
        }
        else{
            ds.current_block_offset = 0;
            ds.bytes_received = 0;
            ds.piece_buffer.clear();
            ds.piece_buffer.assign(piece_size(tf, index), '\0');
            fill_pipeline(sock, ds, tf);
        }
    }

    return false;
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

PendingRequest send_request(int sock, uint32_t piece_index, uint32_t block_offset, long long block_length) {
    
    PendingRequest request;
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

    request.piece_index = piece_index;
    request.block_offset = block_offset;
    request.block_length = block_length;

    return request;
}

void run_message_loop(int sock, TorrentFile tf, std::ofstream &outfile, GlobalTorrentState& gs) {

    PeerState ds;
    ds.socket_fd = sock;
    while (true) { 
        if (gs.done) break;

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
        if (handle_message(sock, msg_id, payload, ds, tf, outfile, gs)) break;
    }

    close(sock);
    
    if (!is_piece_downloaded(gs, ds.current_piece_index)) {
        std::lock_guard<std::mutex> lock(gs.queue_mutex);
        gs.missing_pieces.push(ds.current_piece_index);
    }
}