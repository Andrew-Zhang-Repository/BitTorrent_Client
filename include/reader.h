#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include "ids.h"

struct BencodeNode; 
using BencodeInt = long long;
using BencodeString = std::string;
using BencodeList = std::vector<std::shared_ptr<BencodeNode>>;
using BencodeDict = std::map<std::string, std::shared_ptr<BencodeNode>>;


struct BencodeNode {
    std::variant<BencodeInt, BencodeString, BencodeList, BencodeDict> value;
};



class BencodeParser {

    public:
        size_t info_start = 0; 
        size_t info_end = 0; 


    public:
        static std::shared_ptr<BencodeNode> parse(const std::string& raw_data);
        
    // Public for testing turn it back to private
    public:
        std::shared_ptr<BencodeNode> decodeElement(const std::string& data, size_t& index);
        BencodeString decodeString(const std::string& data, size_t& index);
        BencodeInt decodeInt(const std::string& data, size_t& index);
        BencodeList decodeList(const std::string& data, size_t& index);
        BencodeDict decodeDict(const std::string& data, size_t& index);
};
