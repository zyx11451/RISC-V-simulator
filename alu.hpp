//
// Created by 123 on 2023/6/19.
//

#ifndef VERY_SIMPLE_RISCV_SIMULATOR_ALU_HPP
#define VERY_SIMPLE_RISCV_SIMULATOR_ALU_HPP
#include "ReorderBuffer.hpp"
class ALU {
public:
    enum mode {
        ADD,SUB,XOR,OR,AND,LSHIFT,CMPLESS,CMPLESSU,RSHIFT,RASHIFT,PCADD,CMPEQUAL,CMPNOEQUAL,CMPNOLESS,CMPNOLESSU
    };
private:
    mode now_calc;
    uint32_t num1;
    uint32_t num2;

public:
    ALU(ReorderBuffer* rob_= nullptr){
        rob=rob_;
    }
    ReorderBuffer* rob;
    bool busy;
    bool finished;//有数出来
    uint32_t now_ans;
    uint32_t* ans;
    uint32_t instruction_rob_entry;//算的是哪条指令的内容(用rob中位置表示)
    //跳转地址的计算被认为是无需ALU花一个周期计算的简单运算，可直接即时算出来
    void exe(){
        switch (now_calc) {
            case ADD:
                now_ans=num1+num2;
                break;
            case SUB:
                now_ans=num1-num2;
                break;
            case XOR:
                now_ans=num1^num2;
                break;
            case OR:
                now_ans=num1|num2;
                break;
            case AND:
                now_ans=num1&num2;
                break;
            case LSHIFT:
                now_ans=num1<<num2;
                break;
            case RSHIFT:
                now_ans=num1>>num2;
                break;
            case RASHIFT:
                now_ans=((int)num1>>num2);
                break;
            case CMPLESSU:
                now_ans=(num1<num2)?1:0;
                break;
            case CMPLESS:
                now_ans=((int)num1<(int)num2)?1:0;
                break;
            case CMPEQUAL:
                now_ans=(num1==num2)?1:0;
                break;
            case CMPNOEQUAL:
                now_ans=(num1!=num2)?1:0;
                break;
            case CMPNOLESS:
                now_ans=((int)num1<(int)num2)?0:1;
                break;
            case CMPNOLESSU:
                now_ans=(num1<num2)?0:1;
                break;
            case PCADD:
                now_ans=num1+(int)num2;
                break;
            default:
                break;
        }
        busy= false;
        finished=true;
    }
    void set_mission(uint32_t in1,uint32_t in2,mode type,uint32_t* target,uint32_t ins_rob_entry){
        busy= true;
        num1= in1;
        num2= in2;
        now_calc = type;
        ans=target;
        instruction_rob_entry=ins_rob_entry;
    }
    void get_out_adjust(){
        *ans=now_ans;
        (*rob)[instruction_rob_entry].rob_state=ReorderBufferElements::write;
        finished= false;
    }
    void run(){
        if(busy) {
            exe();
            return;
        }
        if(finished) get_out_adjust();
    }
    void flush(){
        busy= false;
        finished=false;
        ans= nullptr;
    }
};

#endif //VERY_SIMPLE_RISCV_SIMULATOR_ALU_HPP
