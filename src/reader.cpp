#include "../include/reader.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>



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
        auto result = decodeElement(data,index);

        if (dict.count(key) > 0) {throw std::runtime_error("duplicate key");}
        if( index >= data.size()) {throw std::runtime_error("to small");}
        dict[key] = result;
        
    }

    if (index >= data.size()){throw std::runtime_error("Missing dictionary value");}
    index++;

    return dict;
}


TorrentFile BencodeParser::populate_torrent(std::shared_ptr<BencodeNode> node){

    TorrentFile return_file;
    BencodeDict dict = std::get<BencodeDict>(node->value);
    BencodeString announce_str = std::get<BencodeString>(dict["announce"]->value);

    BencodeDict dict_info = std::get<BencodeDict>(dict["info"]->value);
    BencodeInt length = std::get<BencodeInt>(dict_info["length"]->value);
    BencodeString name = std::get<BencodeString>(dict_info["name"]->value);
    BencodeInt piece_length = std::get<BencodeInt>(dict_info["piece length"]->value);
    BencodeString pieces = std::get<BencodeString>(dict_info["pieces"]->value);
    
    return_file.announce_url = announce_str;
    return_file.name = name;
    return_file.length = length;
    return_file.piece_length = piece_length;
    return_file.pieces_hashes = pieces;

    return return_file;

}


int main() {
    std::string test_data = "d8:announce14:http://tracker4:infod4:name8:test.txt6:lengthi12345e12:piece lengthi16384e6:pieces20:12345678901234567890ee";
    BencodeParser test;
    size_t index = 0;
    auto node = test.decodeElement(test_data, index);
    printNode(*node);
    test.populate_torrent(node);
    std::cout << '\n';
}




