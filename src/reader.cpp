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
    if (!(data.find(':') != std::string::npos)){
        throw std::runtime_error("No colon");
    }
    
    // Get byte val and advance index
    int end = data.find(":",index);
    size_t length = end - index; 
    int result = std::stoi(data.substr(index, length));
    index = end + 1;

    // Read bytes from da extracted result and advance index
    std::string extracted = data.substr(index, result);
    index = index + result;


    return extracted;


}

BencodeInt BencodeParser::decodeInt(const std::string& data, size_t& index){

    if (data[index] != 'i')
        throw std::runtime_error("Expected integer");

    index++;  

    if (data[index]=='0'){
        throw std::runtime_error("Leading 0");
    }
    
    if(index >= data.size()) throw std::runtime_error("Too Small");

    if(index >= data.size() || data[index] < '0' || data[index] > '9') {
		throw std::runtime_error("wrong number");
	}

    if (data[data.size() - 1] != 'e'){
        throw std::runtime_error("No exit char");
    }

    size_t end = data.find('e', index);

    std::cout << data.size() - 1<< std::endl; 
    int value = std::stoi(data.substr(index, end - index));
    index = end + 1;       

    return value;

}

BencodeList BencodeParser::decodeList(const std::string& data, size_t& index){

    if (data[data.size() - 1] != 'e'){
        throw std::runtime_error("No exit char");
    }

    index++; 
    BencodeList list;
    
    while (index < data.length() && data[index] != 'e') {
        std::shared_ptr<BencodeNode> element = decodeElement(data, index);
        list.push_back(element);
    }
    
    index++; 
    
    return list;
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

    while (index < data.size() && data[index] != 'e'){
        auto key = decodeString(data, index);
        auto result = decodeElement(data,index);
        dict[key] = result;
    }

    index++;

    return dict;
}

int main() {
    std::string test_data = "11:cheesecake";
    BencodeParser test;
    size_t index = 0;
    auto node = test.decodeElement(test_data, index);

    printNode(*node);
    std::cout << '\n';

  
  
}




