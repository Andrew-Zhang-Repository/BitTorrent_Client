#include "../include/reader.h"
#include "../include/torrent_engine.h"
#include "../include/peer.h"
#include "../include/requests.h"
#include "../include/message.h"
#include "../include/parrallel.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iterator>
#include <thread>

void printNode(const BencodeNode& node);

void printList(const BencodeList& list)
{
    std::cout << "[";

    bool first = true;
    for (const auto& element : list)
    {
        if (!first)
            std::cout << ", ";

        printNode(*element);
        first = false;
    }

    std::cout << "]";
}

void printDict(const BencodeDict& dict)
{
    std::cout << "{";

    bool first = true;
    for (const auto& [key, value] : dict)
    {
        if (!first)
            std::cout << ", ";

        std::cout << '"' << key << "\": ";
        printNode(*value);

        first = false;
    }

    std::cout << "}";
}

void printNode(const BencodeNode& node)
{
    std::visit([](const auto& value)
    {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, BencodeInt>)
        {
            std::cout << value;
        }
        else if constexpr (std::is_same_v<T, BencodeString>)
        {
            std::cout << '"' << value << '"';
        }
        else if constexpr (std::is_same_v<T, BencodeList>)
        {
            printList(value);
        }
        else if constexpr (std::is_same_v<T, BencodeDict>)
        {
            printDict(value);
        }

    }, node.value);
}

BencodeString BencodeParser::decodeString(const std::string& data,size_t& index){
   

    size_t end = data.find(":",index);
    for (size_t i = index ; i < end; i++){
        if (std::isdigit(static_cast<unsigned char>(data[i])) == false){
            throw std::runtime_error("Non digit characters detected before colon");
        }
    }

    if (end == std::string::npos)
        throw std::runtime_error("No colon in string");

    if (end == index)
        throw std::runtime_error("Empty string length");

    if (data[index] == '0' && end != index + 1)
        throw std::runtime_error("Leading zero in string length");
    
  
    
    size_t length = end - index; 
    int result = std::stoll(data.substr(index, length));
    index = end + 1;

    if (index >= data.size()){
        throw std::runtime_error("String extends past end of input");
    }

    // Read bytes from da extracted result and advance index
    std::string extracted = data.substr(index, result);
    index = index + result;


    return extracted;


}

BencodeInt BencodeParser::decodeInt(const std::string& data, size_t& index){

    if (data[index] != 'i')
        throw std::runtime_error("Expected integer");

    index++;
               
    size_t start = index;

    if (data[index] == '-')
        index++;

    if (!std::isdigit(static_cast<unsigned char>(data[index])))
    {
        throw std::runtime_error("Expected integer after signs");
    }

    if (data[index] == '0' && data[index + 1] != 'e'){
        throw std::runtime_error("Leading 0");
    }
    
    size_t end = data.find('e', start);
    if (index >= data.size()) throw std::runtime_error("No e found at the end for ints");
    int value = std::stol(data.substr(start, end - start));
    index = end + 1;       

    return value;

}

BencodeList BencodeParser::decodeList(const std::string& data, size_t& index){

     if (data[index] != 'l'){
        throw std::runtime_error("Expected a list");
    }
    index++; 

    if (index >= data.size()){
        throw std::runtime_error("Unexpected end of input for list");
    }

    BencodeList list;
    
    while (index < data.length() && data[index] != 'e') {
        std::shared_ptr<BencodeNode> element = decodeElement(data, index);
        list.push_back(element);
    }

    if (index >= data.size()) throw std::runtime_error("No e found at the end for list");
    
    index++; 
    
    return list;
}



std::shared_ptr<BencodeNode> BencodeParser::decodeElement(const std::string& data, size_t& index){

    if (index >= data.size())
    throw std::runtime_error("Unexpected end of input when decoding element");

    char c = data[index];

   
    auto node = std::make_shared<BencodeNode>();

    if (c == 'i'){node->value  = decodeInt(data, index);} 
    else if (c == 'l'){node->value  = decodeList(data, index);}
    else if (c == 'd'){node->value  = decodeDict(data, index);}
    else if (c >= '0' && c <= '9'){node->value = decodeString(data, index);}
    else{ throw std::runtime_error("Invalid character read"); }
    
    return node;
}

BencodeDict BencodeParser::decodeDict(const std::string& data, size_t& index){

    if (data[index] != 'd'){
        throw std::runtime_error("Expected a dictionary");
    }

    index = index + 1;

    if (index >= data.size()){
        throw std::runtime_error("Unexpected end of input for dictionary");
    }

    BencodeDict dict = {};

    while (index < data.size() && data[index] != 'e'){

        std::string key = decodeString(data, index);
        if (key == "info")
        {
            info_start = index;
            auto result = decodeElement(data, index);
            info_end = index;
            dict[key] = result;
            
        }
        else
        {
            auto result = decodeElement(data, index);
            
            if (dict.count(key) > 0) {throw std::runtime_error("duplicate key");}
            if( index >= data.size()) {throw std::runtime_error("to small");}
            dict[key] = result;
        }

        
        
    }

    if (index >= data.size()){throw std::runtime_error("Missing dictionary value");}
    index++;

    return dict;
}




std::string readTorrentFile(const std::string& filePath) {

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open torrent file.");
    }

    file.unsetf(std::ios::skipws);

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}


int main() {
    std::string test_data = readTorrentFile("small_file.torrent");
    BencodeParser test;
    size_t index = 0;
    auto node = test.decodeElement(test_data, index);
    Torrent torrent;
    TorrentFile tf = torrent.populate_torrent(node);
    std::cout << tf.name << std::endl;
    std::string sub = test_data.substr(test.info_start, test.info_end  - test.info_start);
    std::string hashed = calculateSHA1(sub);
    std::string peer_id = get_peer("-CC0001-");
    tf.info_hash = torrent.url_encode(hashed); // Url encode the hash
    tf.peer_id = torrent.url_encode(peer_id);
    std::cout<< "Total pieces: " + std::to_string(tf.pieces.length());

    std::string tracker_url = tf.announce_url + 
    "?info_hash=" + tf.info_hash +
    "&peer_id="   + tf.peer_id +
    "&port=6881" +
    "&uploaded=0" +
    "&downloaded=0" +
    "&left="      + std::to_string(tf.length) +
    "&compact=1"; 

    std::string response = get_response(tracker_url);
    BencodeDict return_dict = extractTrackerDictionary(response,test);
    BencodeInt interval = 0;
    BencodeString peers = "";
    get_interval_peers(return_dict,interval,peers);
    printDict(return_dict);
    std::cout << interval << std::endl;
    std::cout << peers << std::endl;

    int total_pieces = tf.pieces.length() / 20;
    GlobalTorrentState gs;
    gs.total_pieces = total_pieces;
    gs.downloaded_pieces.assign((total_pieces + 7) / 8, 0);
    for (int i = 0; i < total_pieces ; i++){
        gs.missing_pieces.push(i);
    }

    std::ofstream outfile(tf.name, std::ios::binary | std::ios::out);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open output file: " << "../" + tf.name << std::endl;
        return 1;
    }
    if (tf.length > 0) {
        outfile.seekp(tf.length - 1);
        outfile.write("\0", 1);
        outfile.seekp(0);
    }

    std::vector<std::thread> workers;
    std::vector<int> sockets;
    if (peers.empty()){
        std::cout << "No other peers are currently online for this torrent." << std::endl;
    }
    else{
        std::vector<peer> peers_list = torrent.extract_peers(peers);
        std::string handshake = get_handshake(hashed,peer_id);
        for (auto i : peers_list){
            std::cout << i.ip +" port: " + std::to_string(i.port) << std::endl;

            int rizz = connect_and_send(handshake,i,hashed);
            if (rizz != -1){
                sockets.push_back(rizz);
                workers.emplace_back(run_message_loop, rizz, tf, std::ref(outfile), std::ref(gs));
            }
            std::cout << rizz << std::endl;
        }

        if (sockets.empty()) {
            std::cout << "Could not connect to any peer." << std::endl;
        } else {
            while (!gs.done) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            for (int sock : sockets) shutdown(sock, SHUT_RDWR);

            for (auto& t : workers) t.join();
        }
    }
    // else get peers logic

    outfile.close();
    std::cout << "\n========================================" << std::endl;
    std::cout << "   DOWNLOAD COMPLETE" << std::endl;
    std::cout << "========================================\n" << std::endl;
    return 0;
}




