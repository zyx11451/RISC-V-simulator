//
// Created by 123 on 2023/6/21.
//

#ifndef MAIN_CPP_REORDERBUFFER_HPP
#define MAIN_CPP_REORDERBUFFER_HPP
#include <queue>
#include "instruction.hpp"
#include "my_queue.hpp"
#include "Register.hpp"
const int rob_size=32;
struct ReorderBufferElements{
    //todo 对状态的维护
    enum state{
        exec,issue,write,commit,load_exec1,load_exec2,load_exec3,store_exec1,store_exec2,store_exec3
    };
    int entry=0;
    bool busy= false;//其实貌似不用
    instruction ins;
    int destination=0;
    uint32_t value=0;
    state rob_state=issue;
    uint32_t tar_address;//S指令专用
};
class ReorderBuffer{
private:
    kingzyx::circular_queue<ReorderBufferElements,rob_size> elements;
    ComputerRegister* cr;
public:

    ReorderBuffer(ComputerRegister* cr_){
        cr=cr_;
    }
    ReorderBufferElements& top(){
        return elements.front();
    }
    ReorderBufferElements& operator[](int in){
        return elements[in];
    }
    void insert(instruction& tar){
        ReorderBufferElements new_rob_e;
        new_rob_e.ins=tar;
        new_rob_e.entry=elements.end_num();
        new_rob_e.busy=true;
        if(!tar.isTypeB()&&(!tar.isTypeS())){
            new_rob_e.destination=tar.rd;
            cr->reorder[tar.rd]=new_rob_e.entry;
        }else{
            new_rob_e.destination=-1;
        }
        elements.push_back(new_rob_e);
    }
    void try_commit_top(){
        //作出一些对于寄存器的操作，然后把保留站指令删掉

    }
    void commit(){

    }
    bool empty(){
        return elements.empty();
    }
    void run(){

    }
};
#endif //MAIN_CPP_REORDERBUFFER_HPP
