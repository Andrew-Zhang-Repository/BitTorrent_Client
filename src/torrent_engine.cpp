
#include "../include/torrent_engine.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <iomanip>

TorrentFile Torrent::populate_torrent(std::shared_ptr<BencodeNode> node){

    TorrentFile return_file;
    BencodeDict dict = std::get<BencodeDict>(node->value);
    BencodeString announce_str = std::get<BencodeString>(dict["announce"]->value);

    BencodeDict dict_info = std::get<BencodeDict>(dict["info"]->value);
    return_file.announce_url = announce_str;
    return_file.name = std::get<BencodeString>(dict_info["name"]->value);
    return_file.piece_length = std::get<BencodeInt>(dict_info["piece length"]->value);
    return_file.pieces_hashes = std::get<BencodeString>(dict_info["pieces"]->value);

    if (dict_info.count("files") != 0) {
        // Consider multi files later
        std::cout << "Detected Multi-File Torrent!" << std::endl;
        return_file.length = 0;
    }
    else{
        return_file.length = std::get<BencodeInt>(dict_info["length"]->value);
    }
    
    return return_file;

}

std::string Torrent::url_encode(const std::string &aString){

    std::string aEncodedString = "";

    for(char aChar : aString){
        if(!std::isalnum((unsigned char)aChar) && aChar != '-' && aChar != '_' && aChar !='.' && aChar != '~'){
            aEncodedString += "%";
            std::stringstream ss;
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)(unsigned char) aChar;
            aEncodedString += ss.str();
        }
        else{
            aEncodedString += aChar;
        }
    }

    return aEncodedString;
}

#include <cstring>
std::vector<peer> Torrent::extract_peers(std::string peers){
    std::vector<peer> peers_list = {};

    if ((peers.size() % 6) != 0){
        throw std::runtime_error("Peers size is invalid must be a multiple of 6");
    }
    std::string ip_str = "";
    for (size_t i = 0; i < peers.length(); i += 6) {
        peer peer_struct;
     
        ip_str += std::to_string(static_cast<int>(static_cast<unsigned char>(peers[i]))) + ".";
        ip_str += std::to_string(static_cast<int>(static_cast<unsigned char>(peers[i + 1 ]))) + ".";
        ip_str += std::to_string(static_cast<int>(static_cast<unsigned char>(peers[i + 2 ]))) + ".";
        ip_str += std::to_string(static_cast<int>(static_cast<unsigned char>(peers[i + 3 ])));

        int port = (static_cast<unsigned char>(peers[i+4]) << 8) | static_cast<unsigned char>(peers[i+5]); // This might be off
        
        peer_struct.ip = ip_str;
        peer_struct.port = port;
        ip_str = "";
        peers_list.push_back(peer_struct);
    }

    return peers_list;
}