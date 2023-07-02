//
// Created by 123 on 2023/6/19.
//

#ifndef VERY_SIMPLE_RISCV_SIMULATOR_REGISTER_HPP
#define VERY_SIMPLE_RISCV_SIMULATOR_REGISTER_HPP
#include <iostream>
class Register {
private:

    uint32_t data;
    //可能添加新东西
public:
    uint32_t operator+(const Register& other) const{
        return data+other.data;
    }
    uint32_t operator-(const Register& other) const{
        return data-other.data;
    }
    void load_num(uint32_t& tar){
        data=tar;
    }
    void load_register(const Register& other){
        data=other.data;
    }
    Register& operator=(int a){
        data=a;
        return *this;
    }
    Register& operator=(const uint32_t &a){
        data=a;
        return *this;
    }
    operator uint32_t (){
        return data;
    }
};
class ComputerRegister{
public:
    Register r[32];
    int reorder[32];//记录对应寄存器的值由哪个rob里指令更新
    ComputerRegister(){
        for(int & i : reorder){
            i=-1;
        }
        for(int i=0;i<32;++i){
            r[i]=0;
        }
    }
    ComputerRegister& operator=(const ComputerRegister& other){
        if(&other== this) return *this;
        for(int i=0;i<32;++i){
            r[i]=other.r[i];
            reorder[i]=other.reorder[i];
        }
        return *this;
    }
    void flush(){
        for(int i=0;i<32;++i){
            reorder[i]=-1;
        }
    }
};
#endif //VERY_SIMPLE_RISCV_SIMULATOR_REGISTER_HPP
