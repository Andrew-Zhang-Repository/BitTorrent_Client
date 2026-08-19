#include "../include/peer.h"
#include <random> 
#include <iostream>
#include <openssl/evp.h>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iterator>
std::string get_peer(std::string identifier){

    if (identifier.size() >= 20){
        throw std::runtime_error("Identifier size too large needs to be less than 20 bytes");
    }

    std::string char_list = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string return_str = identifier;


    std::random_device rd;  
    std::mt19937 gen(rd()); 

   
    std::uniform_int_distribution<size_t> distr(0, char_list.length() - 1);

    while (return_str.size() != 20){
        size_t randomIndex = distr(gen);
        char addchar = char_list[randomIndex];

        return_str += addchar;
        
    }

    return return_str;

}


std::string calculateSHA1(const std::string& data) {
  
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        throw std::runtime_error("Failed to create OpenSSL context");
    }

    if (EVP_DigestInit_ex(context, EVP_sha1(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to initialize SHA-1");
    }

    if (EVP_DigestUpdate(context, data.c_str(), data.length()) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to update SHA-1 hash");
    }

    if (EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to finalize SHA-1 hash");
    }

    EVP_MD_CTX_free(context);

    // Hex to make the hash readable maybe need it later
    /*std::stringstream hexStream;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return hexStream.str();*/

    return std::string(reinterpret_cast<char*>(hash), lengthOfHash);
}

