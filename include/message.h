
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
#include "ids.h"
#include <vector>
#include "../include/torrent_engine.h"
#include "../include/peer.h"
 #include <fstream>
 #include <cstdlib>
#pragma once
struct DownloadState;
struct DownloadState {
    uint32_t current_piece_index = 0;
    uint32_t current_block_offset = 0;
    std::string piece_buffer = "";
};


void handle_message(int sock, message_code id, const std::string& payload, DownloadState &ds, TorrentFile tf);
std::string recv_exact(int n,int sock);
message_code get_code(int id);
void send_interested(int sock);
void run_message_loop(int sock, TorrentFile tf);
void send_request(int sock, uint32_t piece_index, uint32_t block_offset, long long block_length);
void retrieve(DownloadState &ds, TorrentFile tf ,const std::string& payload, int sock, std::ofstream &outfile);
void handle_message(int sock, message_code id, const std::string& payload, DownloadState &ds, TorrentFile tf, std::ofstream &outfile);