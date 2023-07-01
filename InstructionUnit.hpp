//
// Created by 123 on 2023/6/25.
//

#ifndef MAIN_CPP_INSTRUCTIONUNIT_HPP
#define MAIN_CPP_INSTRUCTIONUNIT_HPP
#include <iostream>
#include <queue>
#include "ReservationStation.hpp"
#include "ReorderBuffer.hpp"
#include "RAM.hpp"
#include "instruction.hpp"
#include "alu.hpp"
#include "Register.hpp"
class InstructionUnit{
//一个周期完成IF，一个周期完成ID
//遇到分支指令时为了计算跳转地址需要更多个周期
private:
    uint32_t now_instruction;
    bool fetched_flag;
    bool decode_complete_flag;
    bool decode_failed_flag;
    std::queue<instruction> to_launch;
    int ins_amount;
    ALU* jump_address_calculate;//跳转指令的目标地址由这里算出来
    memory* ComputerMemory;
    ComputerRegister* r;
public:
    InstructionUnit(memory* m_,ComputerRegister* r_){
        fetched_flag= false;
        decode_complete_flag=false;
        decode_failed_flag=false;
        now_instruction=0;
        ins_amount=0;
        ComputerMemory=m_;
        r=r_;
    }
    void run(uint32_t& pc,memory& m_,ReservationStation* rs){
        //每个时钟周期执行什么
        //todo 涉及到跳转时会非常复杂
        if(fetched_flag){
            decode();
            launch(rs);
        }else{
            if(decode_complete_flag) fetch(pc,m_);
        }
    };
    void decode(){
        ++ins_amount;
        instruction ans;
        ans.instruction_ID=ins_amount;
        decode_complete_flag= true;
        if(now_instruction==0x0ff00513) {
            ans.type=Null;
            to_launch.push(ans);
            return ;
        }
        unsigned int type_part1=now_instruction&0x7f;
        unsigned int type_part2=(now_instruction>>12)&0x7;
        unsigned int type_part3=(now_instruction>>25)&0x7f;
        ans.rd=(int)((now_instruction>>7)&0x1f);
        ans.rs1=(now_instruction>>15)&0x1f;
        ans.rs2=(now_instruction>>20)&0x1f;
        switch (type_part1) {
            case 0x37:
                ans.type=Lui;
                ans.immediate=(now_instruction>>12)<<12;
                break;
            case 0x17:
                ans.type=Auipc;
                ans.immediate=(now_instruction>>12)<<12;
                break;
            case 0x6f:
                ans.type=Jal;
                ans.offset=(((now_instruction>>12)&0xff)<<12)|(((now_instruction>>20)&0x1)<<11)|
                        (((now_instruction>>21)&0x3ff)<<1)|(((now_instruction>>31)&0x1)<<20);
                jump_address_calculate->set_mission(ComputerMemory->pc,ans.offset,ALU::PCADD,&(ans.immediate),114514);
                break;
            case 0x67:
                ans.type=Jalr;
                ans.offset=now_instruction>>20;
                //这里相当于只有一个位置的RS,在对应寄存器结果提交之前一直尝试decode
                if(r->reorder[ans.rs1]!=-1){
                    decode_failed_flag=true;
                }else{
                    decode_failed_flag= false;
                    jump_address_calculate->set_mission(r->r[ans.rs1],ans.offset,ALU::PCADD,&(ans.immediate),114514);
                }
                break;
            case 0x63:
                ans.offset=(((now_instruction>>7)&0x1)<<11)|(((now_instruction>>8)&0xf)<<1)|
                              (((now_instruction>>25)&0x3f)<<5)|(((now_instruction>>31)&0x1)<<12);
                jump_address_calculate->set_mission(ComputerMemory->pc,ans.offset,ALU::PCADD,&(ans.immediate),114514);
                switch (type_part2) {
                    case 0x0:
                        ans.type=Beq;
                        break;
                    case 0x1:
                        ans.type=Bne;
                        break;
                    case 0x4:
                        ans.type=Blt;
                        break;
                    case 0x5:
                        ans.type=Bge;
                        break;
                    case 0x6:
                        ans.type=Bltu;
                        break;
                    case 0x7:
                        ans.type=Bgeu;
                        break;
                    default:
                        ans.type=Error;
                        std::cout<<"指令格式出问题："<<std::hex<<now_instruction<<'\n';
                        break;
                }
                break;
            case 0x3:
                ans.offset=now_instruction>>20;
                switch (type_part2) {
                    case 0x0:
                        ans.type=Lb;
                        break;
                    case 0x1:
                        ans.type=Lh;
                        break;
                    case 0x2:
                        ans.type=Lw;
                        break;
                    case 0x4:
                        ans.type=Lbu;
                        break;
                    case 0x5:
                        ans.type=Lhu;
                        break;
                    default:
                        ans.type=Error;
                        std::cout<<"指令格式出问题："<<std::hex<<now_instruction<<'\n';
                        break;
                }
                break;
            case 0x23:
                ans.offset=((now_instruction>>7)&0x1f)|((now_instruction>>25)<<5);
                switch (type_part2) {
                    case 0x0:
                        ans.type=Sb;
                        break;
                    case 0x1:
                        ans.type=Sh;
                        break;
                    case 0x2:
                        ans.type=Sw;
                        break;
                    default:
                        ans.type=Error;
                        std::cout<<"指令格式出问题："<<std::hex<<now_instruction<<'\n';
                        break;
                }
                break;
            case 0x13:
                ans.immediate=now_instruction>>20;
                switch (type_part2) {
                    case 0x0:
                        ans.type=Addi;
                        break;
                    case 0x1:
                        ans.type=Slli;
                        break;
                    case 0x2:
                        ans.type=Slti;
                        break;
                    case 0x3:
                        ans.type=Sltiu;
                        break;
                    case 0x4:
                        ans.type=Xori;
                        break;
                    case 0x5:
                        if(type_part3==0x0) ans.type=Srli;
                        else ans.type=Srai;
                        break;
                    case 0x6:
                        ans.type=Ori;
                        break;
                    case 0x7:
                        ans.type=Andi;
                        break;
                    default:
                        ans.type=Error;
                        std::cout<<"指令格式出问题："<<std::hex<<now_instruction<<'\n';
                        break;
                }
                break;
            case 0x33:
                switch (type_part2) {
                    case 0x0:
                        if(type_part3==0x0) ans.type=Add;
                        else ans.type=Sub;
                        break;
                    case 0x1:
                        ans.type=Sll;
                        break;
                    case 0x2:
                        ans.type=Slt;
                        break;
                    case 0x3:
                        ans.type=Sltu;
                        break;
                    case 0x4:
                        ans.type=Xor;
                        break;
                    case 0x5:
                        if(type_part3==0x0) ans.type=Srl;
                        else ans.type=Sra;
                        break;
                    case 0x6:
                        ans.type=Or;
                        break;
                    case 0x7:
                        ans.type=And;
                        break;
                    default:
                        ans.type=Error;
                        std::cout<<"指令格式出问题："<<std::hex<<now_instruction<<'\n';
                        break;
                }
                break;
            default:
                ans.type=Error;
                std::cout<<"指令格式出问题："<<std::hex<<now_instruction<<'\n';
                break;

        }
        to_launch.push(ans);
        fetched_flag= false;
        if((!(jump_address_calculate->busy))&&(!decode_failed_flag)) decode_complete_flag=true;
        else decode_complete_flag=false;
    };
    void fetch(uint32_t& address,memory& m_){
        now_instruction=(m_[int(address)]<<24)|(m_[int(address)+1]<<16)|(m_[int(address)+2]<<8)|(m_[int(address)+3]);
        fetched_flag=true;
    };
    void launch(ReservationStation* rs){
        //todo
        //todo 等跳转地址计算完成后根据预测切换pc
        //等会把待发射队列改成有长度的
    }
};
#endif //MAIN_CPP_INSTRUCTIONUNIT_HPP
