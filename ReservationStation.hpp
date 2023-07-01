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
const int reservation_station_size=32;
struct ReservationStationElement{
    bool busy;
    uint32_t instruction_ID;
    order_type op;
    uint32_t vj,vk;
    int qj,qk;//-1是没有
    int destination;
    uint32_t A;//跳转类指令的目标地址
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
public:
    ReservationStation(ComputerRegister* cr_,ReorderBuffer* rob_,memory* m_){
        cr=cr_;
        rob=rob_;
        ComputerMemory=m_;
        num=0;
    }
    void insert(instruction& tar){
        rob->insert(tar);
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
                data[a].vj=0;
            }
            if(cr->reorder[tar.rs2]==-1){
                //寄存器已是最新值
                data[a].vk=cr->r[tar.rs2];
                data[a].qk=-1;
            }else{
                //寄存器有依赖
                data[a].qk=cr->reorder[tar.rs2];
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
                data[a].vj=0;
            }
            if(cr->reorder[tar.rs2]==-1){
                //寄存器已是最新值
                data[a].vk=cr->r[tar.rs2];
                data[a].qk=-1;
            }else{
                //寄存器有依赖
                data[a].qk=cr->reorder[tar.rs2];
                data[a].vk=0;
            }
            data[a].A=tar.immediate;
        }else if(tar.isTypeJ()){
            data[a].vj=tar.immediate;
            data[a].vk=ComputerMemory->pc;
            data[a].qj=-1;
            data[a].qk=-1;
            data[a].A=0;//需要后续送到算地址的ALU里进行计算
        }
        if(!tar.isTypeB()&&(!tar.isTypeS())) data[a].destination=cr->reorder[tar.rd];
        else data[a].destination=-1;
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

    };
    void execute_U(ReservationStationElement& tar){
        //U指令的执行不需要alu
        if(tar.op == Lui) {
            (*rob)[tar.destination].value=tar.vj;
        }else if(tar.op == Auipc){
            (*rob)[tar.destination].value=tar.vj+ComputerMemory->pc;
        }
    }
    void execute_B(ReservationStationElement& tar){

    }
    void execute_J(ReservationStationElement& tar){
     //跳转指令直接在instructionUnit里完成
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
};
#endif //MAIN_CPP_RESERVATIONSTATION_HPP
