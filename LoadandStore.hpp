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
    bool busy;
    uint32_t instruction_ID;//越小越先
    order_type op;
    uint32_t vj,vk;
    int qj,qk;//-1是没有
    int destination;
    uint32_t A;//读写类指令的目标地址
    int executing_stage=0;//访存操作需要三个时钟周期，记录目前是第几个时钟周期
    bool address_calculated_flag=false;
    enum status{
        exec,waiting,finished
    };
    status now_status;
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
    ALU* load_and_store_address_calculate;//这玩意不涉及到指令的后续读取，所以不放在InstructionUnit里卡住decode
    bool store_executing_flag=false;//是否有store操作执行却尚未提交，如果该flag为true，且接下来的指令为S类型指令，则不会执行直到该flag变为false
    uint32_t now_executing_store_ID=0;//可和上面并在一起，不过反正基本不考虑时间优化，就先放着吧
    uint32_t now_storing_address=0;
    LoadAndStoreReservationStationElement* now_calculated= nullptr;
public:
    //两个store之间的load可以乱序执行，store与store必须严格顺序
    void insert(instruction& tar){
        rob->insert(tar);
        ++num;
        LoadAndStoreReservationStationElement e;
        e.busy= true;
        e.op=tar.type;
        e.instruction_ID=tar.instruction_ID;
        if(tar.isTypeI()){
            //是load指令
            e.destination=cr->reorder[tar.rd];
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
                e.vj=0;
            }
            if(cr->reorder[tar.rs2]==-1){
                //寄存器已是最新值
                e.vk=cr->r[tar.rs2];
                e.qk=-1;
            }else{
                //寄存器有依赖
                e.qk=cr->reorder[tar.rs2];
                e.vk=0;
            }
            e.A=tar.offset;//注意此时需要相加的是A和vj
        }
    }
    message_from_LoadAndStoreReservationStation run(bool former_store_committed,vector<int>& last_writen_entry,vector<uint32_t>& last_writen_value,ReorderBufferElements& last_committed){
        //上一个周期是否有store指令提交（决定自己能否推进store之后的指令），上一个周期完成Rob中value值的更新的指令的位置，与之对应的更新的值，上一 个周期刚刚被提交的指令
        //每次运行只依靠:自己内部的变量、run函数的参数、寄存器的旧状态。其中run函数的参数由上一个状态的CDB传入
        if(former_store_committed) store_executing_flag = false;
        //改qj,qk,vj,vk
        for(int i=0;i<32;++i){
            if(!data[i].busy) continue;
            for(int j=0;j<=last_writen_entry.size();++j){
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
        try_execute();
        //TODO 还需要整理data（通过接受被提交的指令相关信息来进行）
        if(last_committed.ins.instruction_ID==data.front().instruction_ID) data.pop_front();//应该没有问题，因为rob和这个都是顺序的
        if(last_committed.ins.isTypeS()) store_executing_flag= false;
        //todo 其返回值要传给这个状态的CDB一些信息
        message_from_LoadAndStoreReservationStation ans;
        ans.full=data.full();
        return ans;

    }
    void try_execute(){
        if(store_executing_flag){
            for(int i=data.begin_num();i<=data.end_num();++i){
                if(data[i].op==Sb||data[i].op==Sh||data[i].op==Sw){
                    if(data[i].now_status==LoadAndStoreReservationStationElement::exec) continue;
                    break;
                }else{
                    if(data[i].now_status!=LoadAndStoreReservationStationElement::finished) execute_L(data[i]);
                }
            }
        }else{
            for(int i=data.begin_num();i<=data.end_num();++i){
                if(data[i].op==Sb||data[i].op==Sh||data[i].op==Sw){
                    if(data[i].now_status==LoadAndStoreReservationStationElement::exec) continue;
                    if(!store_executing_flag){
                        if(data[i].now_status!=LoadAndStoreReservationStationElement::finished) execute_S(data[i]);
                        store_executing_flag=true;//在接受到来自Rob的Store被提交信息后变为false，在run函数中执行此操作
                    }else{
                        break;
                    }
                }else{
                    if(data[i].now_status!=LoadAndStoreReservationStationElement::finished) execute_L(data[i]);
                }
            }
        }
    }
    //Load里面加对于读的地址是否被Store占用的判定
    void execute_L(LoadAndStoreReservationStationElement& tar){
        tar.now_status=LoadAndStoreReservationStationElement::exec;
        if(tar.op==Lhu){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                if(tar.A==now_storing_address&&now_executing_store_ID<tar.instruction_ID){
                    //发生读后写
                    return;
                }
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=((*ComputerMemory)[tar.A]<<8)|(*ComputerMemory)[tar.A+1];
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                //把rs1和offset送进alu里
                //先看有没有从alu里出来的数，可能要记一下当前正在被算的指令是哪一个
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag = true;
                }else{
                    //如果alu正在busy，则过不了try_execute
                    //为alu设定计算任务
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }else if(tar.op==Lh){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                if(tar.A==now_storing_address&&now_executing_store_ID<tar.instruction_ID){
                    //发生读后写
                    return;
                }
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=(int)(((*ComputerMemory)[tar.A]<<8)|(*ComputerMemory)[tar.A+1]);
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                //把rs1和offset送进alu里
                //先看有没有从alu里出来的数，可能要记一下当前正在被算的指令是哪一个
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag = true;
                }else{
                    //如果alu正在busy，则过不了try_execute
                    //为alu设定计算任务
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }else if(tar.op==Lb){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                if(tar.A==now_storing_address&&now_executing_store_ID<tar.instruction_ID){
                    //发生读后写
                    return;
                }
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=(int)((*ComputerMemory)[tar.A]);
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                //把rs1和offset送进alu里
                //先看有没有从alu里出来的数，可能要记一下当前正在被算的指令是哪一个
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag = true;
                }else{
                    //如果alu正在busy，则过不了try_execute
                    //为alu设定计算任务
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }else if(tar.op==Lbu){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                if(tar.A==now_storing_address&&now_executing_store_ID<tar.instruction_ID){
                    //发生读后写
                    return;
                }
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=(*ComputerMemory)[tar.A];
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                //把rs1和offset送进alu里
                //先看有没有从alu里出来的数，可能要记一下当前正在被算的指令是哪一个
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag = true;
                }else{
                    //如果alu正在busy，则过不了try_execute
                    //为alu设定计算任务
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }else if(tar.op==Lw){
            if(tar.address_calculated_flag){
                //地址算完，进行读取操作
                if(tar.A==now_storing_address&&now_executing_store_ID<tar.instruction_ID){
                    //发生读后写
                    return;
                }
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].value=((*ComputerMemory)[tar.A]<<24)|((*ComputerMemory)[tar.A+1]<<16)|((*ComputerMemory)[tar.A+2]<<8)|((*ComputerMemory)[tar.A+3]);
                    tar.now_status=LoadAndStoreReservationStationElement::finished;
                }
            }else{
                //把rs1和offset送进alu里
                //先看有没有从alu里出来的数，可能要记一下当前正在被算的指令是哪一个
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag = true;
                }else{
                    //如果alu正在busy，则过不了try_execute
                    //为alu设定计算任务
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }
    }
    void execute_S(LoadAndStoreReservationStationElement& tar){
        tar.now_status=LoadAndStoreReservationStationElement::exec;
        if(tar.op == Sb){
            //如果有其它s类操作没提交，则会在try_execute环节被过掉
            if(tar.address_calculated_flag){
                //存储内存地址已计算完毕，可以执行
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].tar_address=tar.A;
                    (*rob)[tar.destination].value=(tar.vk)|0xff;
                }
            }else{
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag=true;
                }else{
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }else if(tar.op == Sh){
            if(tar.address_calculated_flag){
                //存储内存地址已计算完毕，可以执行
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].tar_address=tar.A;
                    (*rob)[tar.destination].value=(tar.vk)|0xffff;
                }
            }else{
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag=true;
                }else{
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }else if(tar.op==Sw){
            if(tar.address_calculated_flag){
                //存储内存地址已计算完毕，可以执行
                tar.executing_stage++;
                if(tar.executing_stage==3){
                    (*rob)[tar.destination].tar_address=tar.A;
                    (*rob)[tar.destination].value=(tar.vk);
                }
            }else{
                if(load_and_store_address_calculate->finished){
                    load_and_store_address_calculate->get_out_adjust();
                    now_calculated->address_calculated_flag=true;
                }else{
                    load_and_store_address_calculate->set_mission(tar.vj,tar.vk,ALU::PCADD,&(tar.A),114514);
                }
            }
        }
    }
};
#endif //MAIN_CPP_LOADANDSTORE_HPP
