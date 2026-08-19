
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <stdexcept>
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
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <vector>
#pragma once
#include "../include/torrent_engine.h"


struct PeerState;
struct GlobalTorrentState;
struct PendingRequest;

struct PendingRequest {
    uint32_t piece_index;
    uint32_t block_offset;
    uint32_t block_length;
};

struct PeerState {
    int socket_fd; 
    uint32_t current_piece_index = 0; 
    uint32_t current_block_offset = 0;
    uint32_t bytes_received = 0;
    std::string piece_buffer = "";
    std::vector<PendingRequest> pending_requests;
    uint32_t pipeline_depth = 10;
};

struct GlobalTorrentState {
    std::queue<uint32_t> missing_pieces;
    std::atomic<uint32_t> pieces_finished{0};
    std::atomic<bool> done{false};
    uint32_t total_pieces = 0;
    std::vector<uint8_t> downloaded_pieces;
    std::mutex file_mutex; 
    std::mutex queue_mutex; 
};
void worker_thread(int sock, GlobalTorrentState& global_state, TorrentFile& tf);
int get_work(GlobalTorrentState& global_state);
void mark_piece_downloaded(GlobalTorrentState& global_state, uint32_t index);
bool is_piece_downloaded(GlobalTorrentState& global_state, uint32_t index);

