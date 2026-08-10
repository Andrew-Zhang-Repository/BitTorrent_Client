#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>


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
        static std::shared_ptr<BencodeNode> parse(const std::string& raw_data);
    // Public for testing turn it back to private
    public:
        static std::shared_ptr<BencodeNode> decodeElement(const std::string& data, size_t& index);
        static BencodeString decodeString(const std::string& data, size_t& index);
        static BencodeInt decodeInt(const std::string& data, size_t& index);
        static BencodeList decodeList(const std::string& data, size_t& index);
        static BencodeDict decodeDict(const std::string& data, size_t& index);
};
