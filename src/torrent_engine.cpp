
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
