#include "../include/reader.h"
#include <iostream>


BencodeString decodeString(const std::string& data,size_t& index){
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

BencodeInt decodeInt(const std::string& data, size_t& index){

}

BencodeList decodeList(const std::string& data, size_t& index){

}


BencodeDict decodeDict(const std::string& data, size_t& index){

}

std::shared_ptr<BencodeNode> decodeElement(const std::string& data, size_t& index){
    
}

int main() {
    
    size_t index = 0;
    std::cout << decodeString("4:spam",index) << std::endl;
    std::cout << index << std::endl; 
}