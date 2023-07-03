//
// Created by 123 on 2023/6/26.
//

#ifndef MAIN_CPP_LOADANDSTORE_HPP
#define MAIN_CPP_LOADANDSTORE_HPP
#include <vector>
#include "memory"
#include "my_queue.hpp"
#include "alu.hpp"
#include "ReorderBuffer.hpp"
#include "Register.hpp"
#include "instruction.hpp"
using std::vector;
struct LoadAndStoreReservationStationElement{
    bool busy= false;
    uint32_t instruction_ID=0;//越小越先
    order_type op;
    uint32_t vj,vk;
    int qj=-1,qk=-1;//-1是没有
    int destination;
    uint32_t A=0;//读写类指令的目标地址
    int executing_stage=0;//访存操作需要三个时钟周期，记录目前是第几个时钟周期
    bool address_calculated_flag=false;
    enum status{
        exec,waiting,finished
    };
    status now_status=waiting;
};
struct message_from_LoadAndStoreReservationStation{
    bool full;//告诉instructionUnit是否可以发射L或S类指令
};
class LoadAndStoreReservationStation{
    //在InstructionUnit里自由发射,指令的协调由这个类进行
private:
    kingzyx::circular_queue<LoadAndStoreReservationStationElement,32> data;
    ComputerRegister* cr;
    ReorderBuffer* rob;
    memory* ComputerMemory;
    int num=0;
    bool executing_flag=false;
    int now_executing_in;
public:
    LoadAndStoreReservationStation(ComputerRegister* cr_= nullptr,ReorderBuffer* rob_= nullptr,memory* m_= nullptr){
        cr=cr_;
        rob=rob_;
        ComputerMemory=m_;
    }
    //两个store之间的load可以乱序执行，store与store必须严格顺序
    void insert(instruction& tar){
        int k=rob->insert(tar);
        ++num;
        LoadAndStoreReservationStationElement e;
        e.busy= true;
        e.op=tar.type;
        e.instruction_ID=tar.instruction_ID;
        e.destination=k;
        if(tar.isTypeI()){
            //是load指令

            if(cr->reorder[tar.rs1]==-1){
                //寄存器已是最新值
                e.vj=cr->r[tar.rs1];
                e.qj=-1;
            }else{
                //寄存器有依赖
                e.qj=cr->reorder[tar.rs1];
                e.vj=0;
            }
            e.vk=tar.offset;
            e.qk=-1;
            e.A=0;
        }else if(tar.isTypeS()){
            //store
            if(cr->reorder[tar.rs1]==-1){
                //寄存器已是最新值
                e.vj=cr->r[tar.rs1];
                e.qj=-1;
            }else{
                //寄存器有依赖
                e.qj=cr->reorder[tar.rs1];
                if(tar.rs1==0){
                    e.qj=-1;
                }
                e.vj=0;
            }
            if(cr->reorder[tar.rs2]==-1){
                //寄存器已是最新值
                e.vk=cr->r[tar.rs2];
                e.qk=-1;
            }else{
                //寄存器有依赖
                e.qk=cr->reorder[tar.rs2];
                if(tar.rs2==0){
                    e.qk=-1;
                }
                e.vk=0;
            }
            e.A=tar.offset;//注意此时需要相加的是A和vj
        }
        data.push_back(e);
    }
    message_from_LoadAndStoreReservationStation run(vector<int>& last_writen_entry,vector<uint32_t>& last_writen_value,ReorderBufferElements& last_committed){
        //上一个周期是否有store指令提交（决定自己能否推进store之后的指令），所有完成Rob中value值的更新的指令的位置，与之对应的更新的值，上一 个周期刚刚被提交的指令
        //每次运行只依靠:自己内部的变量、run函数的参数、寄存器的旧状态。其中run函数的参数由上一个状态的CDB传入
        //改qj,qk,vj,vk
        for(int i=data.begin_num();i%32!=data.end_num();++i){
            if(i>31) i%=32;
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

        //还需要整理data（通过接受被提交的指令相关信息来进行）
        if(last_committed.ins.instruction_ID==data.front().instruction_ID&&data.front().instruction_ID!=0) data.pop_front();//应该没有问题，因为rob和这个都是顺序的
        if(last_committed.ins.isTypeS()||last_committed.ins.isLoad()) executing_flag= false;
        try_execute();
        message_from_LoadAndStoreReservationStation ans;
        ans.full=data.full();
        return ans;

    }
    void try_execute(){
        if(executing_flag){
            execute(data[now_executing_in]);
        }
        else{
            if(!data.empty()){
                if(data.front().qj==-1&&data.front().qk==-1){
                    executing_flag =true;
                    now_executing_in=data.begin_num();
                    execute(data[now_executing_in]);
                }

            }
        }
    }
    void execute(LoadAndStoreReservationStationElement& tar){
        tar.now_status=LoadAndStoreReservationStationElement::exec;
        if((*rob)[tar.destination].rob_state==ReorderBufferElements::issue) (*rob)[tar.destination].rob_state=ReorderBufferElements::exec;
        if(tar.op==Lhu){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=((*ComputerMemory)[tar.A+2]<<8)|(*ComputerMemory)[tar.A+3];
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                tar.A=tar.vj+tar.vk;
                tar.address_calculated_flag= true;
            }
        }else if(tar.op==Lh){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=(short )((unsigned short )((*ComputerMemory)[tar.A+2]<<8)|(unsigned short)(*ComputerMemory)[tar.A+3]);
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                tar.A=tar.vj+tar.vk;
                tar.address_calculated_flag= true;
            }
        }else if(tar.op==Lb){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                tar.executing_stage++;
                if(tar.executing_stage==3){

                    //可能会改错
                    if(tar.A%4==0) (*rob)[tar.destination].value=(char)(*ComputerMemory)[tar.A+3];
                    else if(tar.A%4==1) (*rob)[tar.destination].value=(char)(*ComputerMemory)[tar.A+1];
                    else if(tar.A%4==2) (*rob)[tar.destination].value=(char)(*ComputerMemory)[tar.A-1];
                    else if(tar.A%4==3) (*rob)[tar.destination].value=(char)(*ComputerMemory)[tar.A-3];
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                tar.A=tar.vj+tar.vk;
                tar.address_calculated_flag= true;
            }
        }else if(tar.op==Lbu){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    if(tar.A%4==0) (*rob)[tar.destination].value=(*ComputerMemory)[tar.A+3];
                    else if(tar.A%4==1) (*rob)[tar.destination].value=(*ComputerMemory)[tar.A+1];
                    else if(tar.A%4==2) (*rob)[tar.destination].value=(*ComputerMemory)[tar.A-1];
                    else if(tar.A%4==3) (*rob)[tar.destination].value=(*ComputerMemory)[tar.A-3];
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                tar.A=tar.vj+tar.vk;
                tar.address_calculated_flag= true;
            }
        }else if(tar.op==Lw){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    //
                    (*rob)[tar.destination].value=
                            (int)((unsigned int)((*ComputerMemory)[tar.A]<<24)|
                                  (unsigned int)((*ComputerMemory)[tar.A+1]<<16)|
                                  (unsigned int)((*ComputerMemory)[tar.A+2]<<8)|
                                  (unsigned int)((*ComputerMemory)[tar.A+3]));
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                tar.A=tar.vj+tar.vk;
                tar.address_calculated_flag= true;
            }
        }
        //
        //
        if(tar.op == Sb){
            if(tar.address_calculated_flag){
                //存储内存地址已计算完毕，可以执行
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].tar_address=tar.A;
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    (*rob)[tar.destination].value=(tar.vk)&0xff;
                }
            }else{
                tar.A=tar.vj+(int)tar.A;
                tar.address_calculated_flag= true;
            }
        }else if(tar.op == Sh){
            if(tar.address_calculated_flag){
                //存储内存地址已计算完毕，可以执行
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].tar_address=tar.A;
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    (*rob)[tar.destination].value=(tar.vk)&0xffff;
                }
            }else{
                tar.A=tar.vj+(int)tar.A;
                tar.address_calculated_flag= true;
            }
        }else if(tar.op==Sw){
            if(tar.address_calculated_flag){
                //存储内存地址已计算完毕，可以执行
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].tar_address=tar.A;
                    (*rob)[tar.destination].rob_state=ReorderBufferElements::write;
                    (*rob)[tar.destination].value=(tar.vk);
                }
            }else{
                tar.A=tar.vj+(int)tar.A;
                tar.address_calculated_flag= true;
            }
        }
    }
    void flush(){
        for(int i=0;i<=31;++i) data[i].busy= false;
        data.clear();
        num=0;
        executing_flag= false;
        now_executing_in=0;
    }
};
#endif //MAIN_CPP_LOADANDSTORE_HPP
