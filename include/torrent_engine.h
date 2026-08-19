
#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include "reader.h"
#include <iostream>
struct TorrentFile;
struct peer;
struct FileEntry;


struct peer{
    std::string ip;
    long long port; 
};

struct FileEntry {
    std::string path;
    uint64_t length;
    uint64_t global_start; 
    uint64_t global_end;
};


struct TorrentFile {
    bool list = false;
    std::vector<std::string> announce_list;
    std::string announce_url; 
    std::string peer_id;
    std::string name;
    long long length;           
    long long piece_length;    
    std::string pieces;  
    std::string info_hash;
    std::vector<FileEntry> files;
    uint64_t total_length;    
};


class Torrent {
    // Public for testing turn it back to private
    public:
        static TorrentFile populate_torrent(std::shared_ptr<BencodeNode> node);
        static std::string url_encode(const std::string &input);
        static std::vector<peer> extract_peers(std::string peers);
};


