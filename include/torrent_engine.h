
#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include "reader.h"
#include <iostream>
struct TorrentFile;
struct TorrentFileEntry;
struct peer;

struct TorrentFileEntry
{
    std::vector<std::string> path;
    long long length;
};

struct peer{
    std::string ip;
    long long port;
};

struct TorrentFile {
  
    std::string announce_url; 
    std::string peer_id;
    std::string name;
    long long length;           
    long long piece_length;    
    std::string pieces_hashes;  
    std::string info_hash;
    std::vector<TorrentFileEntry> files;     
};


class Torrent {
    // Public for testing turn it back to private
    public:
        static TorrentFile populate_torrent(std::shared_ptr<BencodeNode> node);
        static std::string url_encode(const std::string &input);
        static std::vector<peer> extract_peers(std::string peers);
};


