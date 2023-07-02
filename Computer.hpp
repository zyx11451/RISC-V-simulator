//
// Created by 123 on 2023/6/21.
//

#ifndef MAIN_CPP_COMPUTER_HPP
#define MAIN_CPP_COMPUTER_HPP
#include "Register.hpp"
#include "InstructionUnit.hpp"
#include "RAM.hpp"
#include "ReservationStation.hpp"
#include "LoadandStore.hpp"
#include "predictor.hpp"
#include "alu.hpp"
//为让每个周期各元件能乱序执行，必须记录上个周期的状态。
struct  CDB{
    //上个周期传来的数据
    message_from_reorder_buffer m_rob;
    message_from_reservation_station mrs;
    message_from_LoadAndStoreReservationStation mls;
};
class ControlUnit{
    long long clock_cycle;
    memory ComputerMemory;
    InstructionUnit ComputerInstructionUnit;
    ALU computer_alu[6];
    Predictor pre;
    ReservationStation rs;
    LoadAndStoreReservationStation ls;
    ReorderBuffer rob;
    ComputerRegister old_r;
    ComputerRegister new_r;
    CDB old_message;
    CDB new_message;
public:
    ControlUnit(){
        ComputerMemory=memory();
        pre=Predictor();
        old_r=ComputerRegister();
        new_r=ComputerRegister();
        ComputerInstructionUnit=InstructionUnit(&ComputerMemory,&old_r,&pre);
        rob=ReorderBuffer(&new_r,&pre);
        rs=ReservationStation(&old_r,&rob,&ComputerMemory);
        ls=LoadAndStoreReservationStation(&old_r,&rob,&ComputerMemory);

    }
    void ini(){
        rs.set_ALU(computer_alu,computer_alu+1,computer_alu+2,computer_alu+3,computer_alu+4,computer_alu+5);
        ComputerMemory.setInstruction();
        for(int i=0;i<=5;++i){
            computer_alu[i].rob=&rob;
        }
    }
    void run(){
        ++clock_cycle;
        //
        old_message=new_message;
        old_r=new_r;
        if(!old_message.m_rob.cm.flush_flag){
            ComputerInstructionUnit.run(old_message.m_rob.full,old_message.mrs.full,old_message.mls.full,&rs,&ls);
            new_message.mrs=rs.run(old_message.m_rob.written_entry,old_message.m_rob.written_value,old_message.m_rob.cm.rob_e);
            new_message.mls=ls.run(old_message.m_rob.written_entry,old_message.m_rob.written_value,old_message.m_rob.cm.rob_e);
            for(int i=0;i<=5;++i){
                computer_alu[i].run();
            }
            new_message.m_rob=rob.run();

        }else{
            new_message=CDB();
            new_r.flush();
            ComputerInstructionUnit.flush();
            rs.flush();
            ls.flush();
            rob.flush();
            for(int i=0;i<=5;++i){
                computer_alu[i].flush();
            }
            if(pre.should_jump) ComputerMemory.pc=pre.jump_pc;
            else ComputerMemory.pc=pre.now_pc;
        }

        //根据new_message更新寄存器状态和内存，或决定下一周期冲刷流水线
        if(new_message.m_rob.cm.register_change){

            new_r.r[new_message.m_rob.cm.destination]=new_message.m_rob.cm.v;
            if(new_r.reorder[new_message.m_rob.cm.destination]==new_message.m_rob.cm.rob_e.entry)
                new_r.reorder[new_message.m_rob.cm.destination]=-1;
            new_r.r[0]=0;
            new_r.reorder[0]=-1;
        }
        if(new_message.m_rob.cm.memory_change){
            //根据S指令的具体类型进行写入

            if(new_message.m_rob.cm.rob_e.ins.type==Sb) ComputerMemory[new_message.m_rob.cm.address]=new_message.m_rob.cm.v&0xff;
            else if(new_message.m_rob.cm.rob_e.ins.type==Sh) {
                ComputerMemory[new_message.m_rob.cm.address]=(new_message.m_rob.cm.v>>8)&0xff;
                ComputerMemory[new_message.m_rob.cm.address+1]=new_message.m_rob.cm.v&0xff;
            }else if(new_message.m_rob.cm.rob_e.ins.type==Sw){
                ComputerMemory[new_message.m_rob.cm.address]=(new_message.m_rob.cm.v>>24)&0xff;
                ComputerMemory[new_message.m_rob.cm.address+1]=(new_message.m_rob.cm.v>>16)&0xff;
                ComputerMemory[new_message.m_rob.cm.address+2]=(new_message.m_rob.cm.v>>8)&0xff;
                ComputerMemory[new_message.m_rob.cm.address+3]=new_message.m_rob.cm.v&0xff;
            }
        }
    }
};
#endif //MAIN_CPP_COMPUTER_HPP
