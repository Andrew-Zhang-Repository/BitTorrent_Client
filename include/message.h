
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
#pragma once
struct DownloadState;
struct DownloadState {
    uint32_t current_piece_index = 0;
    uint32_t current_block_offset = 0;
};


void handle_message(int sock, message_code id, const std::string& payload, DownloadState &ds, long long tor_length);
std::string recv_exact(int n,int sock);
message_code get_code(int id);
void send_interested(int sock);
void run_message_loop(int sock, long long tor_length);
void send_request(int sock, uint32_t piece_index, uint32_t block_offset, long long block_length);