#include "../include/reader.h"
#include "../include/torrent_engine.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>


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
    int result = std::stoi(data.substr(index, length));
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
    int value = std::stoi(data.substr(start, end - start));
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




int main() {
    std::string test_data = "d8:announce14:http://tracker4:infod4:name8:test.txt6:lengthi12345e12:piece lengthi16384e6:pieces20:12345678901234567890ee";
    BencodeParser test;
    size_t index = 0;
    auto node = test.decodeElement(test_data, index);
    Torrent torrent;
    TorrentFile tf;
    std::string sub = test_data.substr(test.info_start, test.info_end  - test.info_start);

    unsigned char hash[20];

    SHA1(
        reinterpret_cast<const unsigned char*>(sub.data()),
        sub.size(),
        hash
    );


    // Just testing for the info hash
    std::stringstream ss;

    for (unsigned char c : hash) {
        ss << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(c);
    }

    std::string info_hash = ss.str();
    tf.info_hash = info_hash;
    std::cout << "Info hash: " << tf.info_hash << '\n';

    printNode(*node);
    std::cout << '\n';
}




