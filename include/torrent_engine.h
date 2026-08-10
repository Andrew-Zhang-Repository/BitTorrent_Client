
#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include "reader.h"

struct TorrentFile;
struct TorrentFileEntry;

struct TorrentFileEntry
{
    std::vector<std::string> path;
    long long length;
};


struct TorrentFile {
  
    std::string announce_url; 
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
};


