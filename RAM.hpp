//
// Created by 123 on 2023/6/21.
//

#ifndef MAIN_CPP_RAM_HPP
#define MAIN_CPP_RAM_HPP
#include <iostream>
#include <string>
using std::cin;
using std::cout;
const int memory_size=409600;
class memory{
private:
    uint8_t *data;

public:
    uint32_t pc;
    memory(){
        data=new uint8_t[memory_size];
    }
    uint8_t& operator[](int in){
        if(in>=memory_size||in<0){
            cout<<"越界\n";
        }
        return data[in];
    }
    ~memory(){
        delete []data;
    }

    void setInstruction(){
        std::string input;
        uint32_t pc_=0;
        int k=3;//3,2,1,0,3,2,1,0...
        int a;
        while(cin>>input){
            if(input[0]=='@'){
                a=0;
                for(int i=1;i<=8;++i){
                    a*=16;
                    a+=(input[i]-'0');
                }
                pc_=a;
            }else{
                a=(input[0]-'0')*16+(input[1]-'0');
                data[pc_+k]=a;
                if(k==0) {
                    k=3;
                    pc_+=4;
                }
                else --k;
            }
        }
    }
};
#endif //MAIN_CPP_RAM_HPP
