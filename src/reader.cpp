#include "../include/reader.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <memory>

BencodeNode decodeElement(const std::string& data, size_t& index);
BencodeString decodeString(const std::string& data, size_t& index);
BencodeInt decodeInt(const std::string& data, size_t& index);
BencodeList decodeList(const std::string& data, size_t& index);
BencodeDict decodeDict(const std::string& data, size_t& index);

BencodeString BencodeParser::decodeString(const std::string& data,size_t& index){

    
    // Get byte val and advance index
    int end = data.find(":");
    size_t length = end - index; 
    int result = std::stoi(data.substr(index, length));
    index = index + end + 1;

    // Read bytes from da extracted result and advance index
    std::string extracted = data.substr(index, result);
    index = index + result;


    return extracted;


}

BencodeInt BencodeParser::decodeInt(const std::string& data, size_t& index){

    if (data[index] != 'i'){
        return 0;
    }
    index = index + 1;
    // i42e
    int start = index;
    int end = data.find("e");
    index = index + end;
    // Get inbetween 
    return std::stoi(data.substr(start, end - start));

}

BencodeList BencodeParser::decodeList(const std::string& data, size_t& index){

    index++; 
    
    while (index < data.length() && data[index] != 'e') {
        std::shared_ptr<BencodeNode> element = decodeElement(data, index);
        list.push_back(element);
    }
    
    index++; 
    auto node = std::make_shared<BencodeNode>();
    node->value = list;
    return node;

}



std::shared_ptr<BencodeNode> BencodeParser::decodeElement(const std::string& data, size_t& index){

    char c = data[index];

    if (c != 'i' && c!= 'l' && c!='d' && !(c >= '0' && c <= '9')){
        throw "Invalid character read";
    }

    auto node = std::make_shared<BencodeNode>();

    if (c == 'i'){node->value  = decodeInt(data, index);}
    if (c == 'l'){node->value  = decodeList(data, index);}
    if (c == 'd'){node->value  = decodeDict(data, index);}
    if (c >= '0' && c <= '9'){node->value = decodeString(data, index);}
    
    return node;
}

BencodeDict BencodeParser::decodeDict(const std::string& data, size_t& index){

    if (data[index] != 'd'){
        BencodeDict empty = {};
        return empty;
    }
    BencodeDict dict = {};

    index = index + 1;

    while (data[index] != 'e'){
        auto key = decodeString(data, index);
        auto result = decodeElement(data,index);
        dict[key] = result;
    }

    index++;

    return dict;
}

int main() {
    BencodeParser test;
    size_t index = 0;
    std::cout << test.decodeInt("i42e",index) << std::endl;
    std::cout << index << std::endl;
}