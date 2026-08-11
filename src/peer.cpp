#include "../include/peer.h"
#include <random> 
#include <iostream>
std::string get_peer(std::string identifier){

    if (identifier.size() >= 20){
        throw std::runtime_error("Identifier size too large needs to be less than 20 bytes");
    }

    std::string char_list = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string return_str = identifier;


    std::random_device rd;  
    std::mt19937 gen(rd()); 

   
    std::uniform_int_distribution<size_t> distr(0, char_list.length() - 1);


    size_t randomIndex = distr(gen);
    char randomChar = char_list[randomIndex];

    while (return_str.size() != 20){
        size_t randomIndex = distr(gen);
        char randomChar = char_list[randomIndex];

        return_str += randomChar;
        
    }

    return return_str;

}

int main(){
    std::cout << get_peer("-GB0001-") << std::endl;
    return 0;
}