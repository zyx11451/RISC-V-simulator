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
#include "predictor.hpp"
#include "LoadandStore.hpp"
class InstructionUnit{
//一个周期完成IF，一个周期完成ID
//遇到分支指令时为了计算跳转地址需要更多个周期
private:
    uint32_t now_instruction;
    uint32_t now_addr;
    bool fetched_flag;
    bool decode_complete_flag;
    bool decode_failed_flag;
    std::queue<instruction> to_launch;
    int ins_amount;
    memory* ComputerMemory;
    ComputerRegister* r;
    Predictor* pre;
    bool jalr_wait= false;
    bool end = false;
public:
    InstructionUnit(memory* m_= nullptr,ComputerRegister* r_= nullptr,Predictor* pre_= nullptr){
        fetched_flag= false;
        decode_complete_flag=true;
        decode_failed_flag=false;
        now_instruction=0;
        ins_amount=0;
        ComputerMemory=m_;
        r=r_;
        pre=pre_;
    }
    void decode(){
        ++ins_amount;
        instruction ans;
        ans.pc=now_addr;
        ans.instruction_ID=ins_amount;
        decode_complete_flag= true;
        if(now_instruction==0xff00513) {
            ans.type=Li;
            end=true;
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
                if(ans.offset>>20) ans.offset|=0xfff00000;
                ans.immediate=ComputerMemory->pc+(int)ans.offset;
                //立刻跳转
                ComputerMemory->pc=ans.immediate;
                break;
            case 0x67:
                ans.type=Jalr;
                ans.offset=now_instruction>>20;
                if(ans.offset>>11) ans.offset|=0xfffff000;
                //这里相当于只有一个位置的RS,在对应寄存器结果提交之前一直尝试decode
                //强行等一回合，使得接收到最新的寄存器信息
                if(!jalr_wait){
                    jalr_wait=true;
                    decode_failed_flag=true;
                }else{
                    if(r->reorder[ans.rs1]!=-1){
                        decode_failed_flag=true;
                    }else{
                        decode_failed_flag= false;
                        ans.immediate=(r->r[ans.rs1]+(int)ans.offset)&(~1);
                        //立刻跳转
                        ComputerMemory->pc=ans.immediate;
                    }
                    jalr_wait= false;
                }

                break;
            case 0x63:
                ans.offset=(((now_instruction>>7)&0x1)<<11)|(((now_instruction>>8)&0xf)<<1)|
                              (((now_instruction>>25)&0x3f)<<5)|(((now_instruction>>31)&0x1)<<12);
                if(ans.offset>>12) ans.offset|=0xffffe000;
                ans.immediate=ComputerMemory->pc+(int)ans.offset;
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
                if(pre->busy){
                    decode_failed_flag= true;
                }else{
                    decode_failed_flag= false;
                    pre->busy=true;
                    pre->now_pc=ComputerMemory->pc+4;
                    pre->jump_pc=ans.immediate;
                    if(pre->predict_jump){
                        ComputerMemory->pc=ans.immediate;
                    }else{
                        ComputerMemory->pc+=4;
                    }
                }

                break;
            case 0x3:
                ans.offset=now_instruction>>20;
                if(ans.offset>>11) ans.offset|=0xfffff000;
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
                if(ans.offset>>11) ans.offset|=0xfffff000;
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
                if(ans.immediate>>11) ans.immediate|=0xfffff000;
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

        fetched_flag= false;
        if(!decode_failed_flag) {

            decode_complete_flag=true;
            to_launch.push(ans);
        }
        else decode_complete_flag=false;
        if(!ans.isTypeB()&&!ans.isTypeJ()&&ans.type!=Jalr){
            ComputerMemory->pc+=4;
        }
    };
    void fetch(uint32_t& address,memory& m_){
        now_instruction=(m_[int(address)]<<24)|(m_[int(address)+1]<<16)|(m_[int(address)+2]<<8)|(m_[int(address)+3]);
        now_addr=address;
        fetched_flag=true;
    };
    void launch(bool rob_full,bool rs_full,bool ls_rs_full,ReservationStation* rs,LoadAndStoreReservationStation* ls){
        if(to_launch.front().isTypeS()||to_launch.front().isLoad()){
            if(!rob_full&&!ls_rs_full){
                ls->insert(to_launch.front());
                to_launch.pop();
            }
        }else{
            if(!rob_full&&!rs_full){
                rs->insert(to_launch.front());
                to_launch.pop();
            }
        }
    }
    void run(bool rob_full,bool rs_full,bool ls_rs_full,ReservationStation* rs,LoadAndStoreReservationStation* ls){
        //fetch和decode交替进行而非同时进行，只要不遇到jalr，就是两个周期发射一条指令(其实可以通过写个little_flush之类的东西来实现流水式)
        if(!end){
            if(!fetched_flag&&decode_complete_flag) {
                fetch(ComputerMemory->pc,*ComputerMemory);
            }else{
                decode();
            }
        }

        if((!to_launch.empty())){
            launch(rob_full,rs_full,ls_rs_full,rs,ls);//根据类型向ls或rs发射一个指令
        }
    }
    void flush(){
        fetched_flag= false;
        decode_complete_flag= true;
        decode_failed_flag=false;
        while(!to_launch.empty()) to_launch.pop();
    }
};
#endif //MAIN_CPP_INSTRUCTIONUNIT_HPP
