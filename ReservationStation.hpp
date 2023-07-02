//
// Created by 123 on 2023/6/21.
//

#ifndef MAIN_CPP_RESERVATIONSTATION_HPP
#define MAIN_CPP_RESERVATIONSTATION_HPP
#include "my_queue.hpp"
#include "instruction.hpp"
#include "Register.hpp"
#include "alu.hpp"
#include "ReorderBuffer.hpp"
#include "RAM.hpp"
#include "predictor.hpp"
#include <vector>
using std::vector;
const int reservation_station_size=32;
struct ReservationStationElement{
    bool busy;
    uint32_t instruction_ID;
    order_type op;
    uint32_t vj,vk;
    int qj,qk;//-1是没有
    int destination;
    uint32_t A;//跳转类指令的目标地址
    bool isTypeU() const{
        return op==Lui||op==Auipc;
    }
    bool isTypeI() const{
        return op==Jalr||op==Lb||op==Lh||op==Lw||op==Lbu||op==Lhu
               ||op==Addi||op==Slti||op==Sltiu||op==Xori||op==Ori||op==Andi
               ||op==Slli||op==Srli||op==Srai;
    }
    bool isCalc() const{
        return op==Addi||op==Slti||op==Sltiu||op==Xori||op==Ori||op==Andi
               ||op==Slli||op==Srli||op==Srai||op==Add||op==Sub||op==Sll||
               op==Slt||op==Sltu||op==Xor||op==Srl||op==Sra||op==Or||op==And;
    }
    bool isTypeB() const{
        return op==Beq||op==Bne||op==Blt||op==Bge||op==Bltu||op==Bgeu;
    }
    bool isTypeJ() const{
        return op==Jal;
    }
    bool isTypeR() const{
        return op==Add||op==Sub||op==Sll||op==Slt||op==Sltu||op==Xor
               ||op==Srl||op==Sra||op==Or||op==And;
    }
};
struct message_from_reservation_station{
    bool full;
};
class ReservationStation{
    //L和S不在这里
private:
    ReservationStationElement data[reservation_station_size];
    ComputerRegister* cr;
    ReorderBuffer* rob;
    memory* ComputerMemory;
    int num;
    ALU* common_calculate[5];
    ALU* jump_condition_calculate;
public:
    ReservationStation(ComputerRegister* cr_= nullptr,ReorderBuffer* rob_= nullptr,memory* m_= nullptr){
        cr=cr_;
        rob=rob_;
        ComputerMemory=m_;
        num=0;
    }
    void set_ALU(ALU* a1,ALU* a2,ALU* a3,ALU* a4,ALU* a5,ALU* a6){
        common_calculate[0]=a1;
        common_calculate[1]=a2;
        common_calculate[2]=a3;
        common_calculate[3]=a4;
        common_calculate[4]=a5;
        jump_condition_calculate=a6;
    }
    void insert(instruction& tar){
        int k=rob->insert(tar);
        if(tar.isTypeJ()||tar.type==Jalr||tar.type==Li){
            //如果是跳转指令，则可以立刻知道其运算结果，都不用放在RS里
            return;
        }
        int a;
        for(a=0;a<=31;++a){
            if(!data[a].busy) break;
        }
        ++num;
        data[a].busy= true;
        data[a].instruction_ID=tar.instruction_ID;
        data[a].op=tar.type;
        //
        if(tar.isTypeU()){
            data[a].vj=tar.immediate;
            data[a].vk=0;
            if(tar.type==Auipc) data[a].vk=tar.pc;
            data[a].qj=-1;
            data[a].qk=-1;
            data[a].A=0;
        }else if(tar.isTypeI()){
            //仅对非Load的i
            if(cr->reorder[tar.rs1]==-1){
                //寄存器已是最新值
                data[a].vj=cr->r[tar.rs1];
                data[a].qj=-1;
            }else{
                //寄存器有依赖

                data[a].qj=cr->reorder[tar.rs1];
                if(tar.rs1==0){
                    data[a].qj=-1;
                }
                data[a].vj=0;
            }
            data[a].vk=tar.immediate;
            data[a].qk=-1;
            data[a].A=0;
        }else if(tar.isTypeR()){
            if(cr->reorder[tar.rs1]==-1){
                //寄存器已是最新值
                data[a].vj=cr->r[tar.rs1];
                data[a].qj=-1;
            }else{
                //寄存器有依赖
                data[a].qj=cr->reorder[tar.rs1];
                if(tar.rs1==0){
                    data[a].qj=-1;
                }
                data[a].vj=0;
            }
            if(cr->reorder[tar.rs2]==-1){
                //寄存器已是最新值
                data[a].vk=cr->r[tar.rs2];
                data[a].qk=-1;
            }else{
                //寄存器有依赖
                data[a].qk=cr->reorder[tar.rs2];
                if(tar.rs2==0){
                    data[a].qk=-1;
                }
                data[a].vk=0;
            }
            data[a].A=0;
        }else if(tar.isTypeB()){

            if(cr->reorder[tar.rs1]==-1){
                //寄存器已是最新值
                data[a].vj=cr->r[tar.rs1];
                data[a].qj=-1;
            }else{
                //寄存器有依赖
                data[a].qj=cr->reorder[tar.rs1];
                if(tar.rs1==0){
                    data[a].qj=-1;
                }
                data[a].vj=0;
            }
            if(cr->reorder[tar.rs2]==-1){
                //寄存器已是最新值
                data[a].vk=cr->r[tar.rs2];
                data[a].qk=-1;
            }else{
                //寄存器有依赖
                data[a].qk=cr->reorder[tar.rs2];
                if(tar.rs2==0){
                    data[a].qk=-1;
                }
                data[a].vk=0;
            }
            data[a].A=tar.immediate;
        }
        data[a].destination=k;
        //

    }
    void erase(instruction& tar){
        int a;
        for(a=0;a<=31;++a){
            if(data[a].instruction_ID==tar.instruction_ID&&data[a].busy){
                data[a].busy= false;
                --num;
                return;
            }
        }
    }
    bool full() const{
        return num==32;
    }
    void try_execute(){
        for(int i=0;i<32;++i){
            if(data[i].busy){
                if(data[i].qj==-1&&data[i].qk==-1){
                    if((*rob)[data[i].destination].rob_state!=ReorderBufferElements::write&&(*rob)[data[i].destination].busy){
                        if(data[i].isTypeU()){
                            execute_U(data[i]);
                        }else if(data[i].isCalc()){
                            int j;
                            for(j=0;j<=4;++j){
                                if((!(common_calculate[j]->busy))&&(!(common_calculate[j]->finished))) break;
                            }
                            if(j==5) continue;//所有alu均被占用
                            else{
                                if((*rob)[data[i].destination].rob_state==ReorderBufferElements::issue)
                                execute_Calc(data[i],common_calculate[j]);
                            }
                        }else if(data[i].isTypeB()){
                            execute_B(data[i]);
                        }
                    }
                }
            }

        }
    };
    void execute_U(ReservationStationElement& tar){
        //U指令的执行不需要alu
        if(tar.op == Lui) {
            (*rob)[tar.destination].value=tar.vj;
        }else if(tar.op == Auipc){
            (*rob)[tar.destination].value=tar.vj+tar.vk;
        }
        //传出rob对应指令状态变成write的信息
        //一切指令运算结果向rob汇报（若rob先run，则rob在下一个周期开始时接受这些数据，并立刻将对应指令的运行阶段改为write，之后在下个周期的提交中把这些新变成write的指令信息广播给其余元件，这样一个周期内运算的结果在两个周期后变得可被其它指令使用），但是本类中状态的修改在当前周期进行
        (*rob)[tar.destination].rob_state=ReorderBufferElements::write;

    }
    void execute_B(ReservationStationElement& tar){
        (*rob)[tar.destination].rob_state=ReorderBufferElements::exec;
        if(!jump_condition_calculate->busy&&(!jump_condition_calculate->finished)){
            if(tar.op==Beq){
                jump_condition_calculate->set_mission(tar.vj,tar.vk,ALU::CMPEQUAL,&((*rob)[tar.destination].value),tar.destination);
            }else if(tar.op==Bne){
                jump_condition_calculate->set_mission(tar.vj,tar.vk,ALU::CMPNOEQUAL,&((*rob)[tar.destination].value),tar.destination);
            }else if(tar.op==Bge){
                jump_condition_calculate->set_mission(tar.vj,tar.vk,ALU::CMPNOLESS,&((*rob)[tar.destination].value),tar.destination);
            }else if(tar.op==Bgeu){
                jump_condition_calculate->set_mission(tar.vj,tar.vk,ALU::CMPNOLESSU,&((*rob)[tar.destination].value),tar.destination);
            }else if(tar.op==Blt){
                jump_condition_calculate->set_mission(tar.vj,tar.vk,ALU::CMPLESS,&((*rob)[tar.destination].value),tar.destination);
            }else if(tar.op==Bltu){
                jump_condition_calculate->set_mission(tar.vj,tar.vk,ALU::CMPLESSU,&((*rob)[tar.destination].value),tar.destination);
            }
        }
    }
    void execute_Calc(ReservationStationElement& tar,ALU* available_alu){
        //以下指令的执行需要alu
        (*rob)[tar.destination].rob_state=ReorderBufferElements::exec;
        if(tar.op == Add||tar.op== Addi){
            available_alu->set_mission(tar.vj,tar.vk,ALU::ADD,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Sub){
            available_alu->set_mission(tar.vj,tar.vk,ALU::SUB,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Slti||tar.op==Slt){
            available_alu->set_mission(tar.vj,tar.vk,ALU::CMPLESS,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Sltiu||tar.op==Sltu){
            available_alu->set_mission(tar.vj,tar.vk,ALU::CMPLESSU,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Xori||tar.op==Xor){
            available_alu->set_mission(tar.vj,tar.vk,ALU::XOR,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Ori||tar.op==Or){
            available_alu->set_mission(tar.vj,tar.vk,ALU::OR,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Andi||tar.op==And){
            available_alu->set_mission(tar.vj,tar.vk,ALU::AND,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Slli||tar.op==Sll){
            available_alu->set_mission(tar.vj,tar.vk,ALU::LSHIFT,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Srli||tar.op==Srl){
            available_alu->set_mission(tar.vj,tar.vk,ALU::RSHIFT,&((*rob)[tar.destination].value),tar.destination);
        }else if(tar.op==Srai||tar.op==Sra){
            available_alu->set_mission(tar.vj,tar.vk,ALU::RASHIFT,&((*rob)[tar.destination].value),tar.destination);
        }
    }
    message_from_reservation_station run(vector<int>& last_writen_entry,vector<uint32_t>& last_writen_value,ReorderBufferElements& last_committed){
        for(int i=0;i<32;++i){
            if(!data[i].busy) continue;
            for(int j=0;j<last_writen_entry.size();++j){
                if(data[i].qj==last_writen_entry[j]){
                    data[i].vj=last_writen_value[j];
                    data[i].qj=-1;
                }
                if(data[i].qk==last_writen_entry[j]){
                    data[i].vk=last_writen_value[j];
                    data[i].qk=-1;
                }
            }
        }
        for(int i=0;i<32;++i){
            if(last_committed.ins.instruction_ID==data[i].instruction_ID&&last_committed.ins.instruction_ID!=0){
                erase(last_committed.ins);
            }
        }
        try_execute();
        message_from_reservation_station ans;
        ans.full=full();
        return ans;
    }
    void flush(){
        num=0;
        for(int i=0;i<32;++i){
            data[i].busy= false;
        }
    }
};
#endif //MAIN_CPP_RESERVATIONSTATION_HPP
