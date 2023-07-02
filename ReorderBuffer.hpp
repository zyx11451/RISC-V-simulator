//
// Created by 123 on 2023/6/21.
//

#ifndef MAIN_CPP_REORDERBUFFER_HPP
#define MAIN_CPP_REORDERBUFFER_HPP
#include <queue>
#include "instruction.hpp"
#include "my_queue.hpp"
#include "Register.hpp"
#include "predictor.hpp"
#include "RAM.hpp"
using std::vector;
const int rob_size=32;
struct ReorderBufferElements{
    enum state{
        exec,issue,write,commit,special
    };
    int entry=0;
    bool busy= false;//其实貌似不用
    instruction ins;
    int destination=0;
    uint32_t value=0;//对于B类指令，0不跳，1跳
    state rob_state=issue;
    uint32_t tar_address;//S指令专用
};
struct CommitMessage{
    ReorderBufferElements rob_e;
    uint32_t v;
    int destination;
    bool register_change= false;//是否需要修改寄存器
    uint32_t address;
    bool memory_change= false;//是否需要修改内存
    bool flush_flag= false;
};
struct message_from_reorder_buffer{
    CommitMessage cm;
    std::vector<int> written_entry;
    std::vector<uint32_t> written_value;
    bool full;
};
class ReorderBuffer{
private:
    kingzyx::circular_queue<ReorderBufferElements,rob_size> elements;
    ComputerRegister* cr;
    Predictor* pre;
public:

    ReorderBuffer(ComputerRegister* cr_= nullptr,Predictor* pre_= nullptr){
        cr=cr_;
        pre=pre_;
    }
    ReorderBufferElements& top(){
        return elements.front();
    }
    ReorderBufferElements& operator[](int in){
        return elements[in];
    }
    int insert(instruction& tar){
        ReorderBufferElements new_rob_e;
        new_rob_e.ins=tar;
        new_rob_e.rob_state=ReorderBufferElements::issue;
        new_rob_e.entry=elements.end_num();
        new_rob_e.busy=true;
        if((!tar.isTypeB())&&(!tar.isTypeS())&&(tar.type!=Li)){
            new_rob_e.destination=tar.rd;
            cr->reorder[tar.rd]=new_rob_e.entry;
        }else{
            new_rob_e.destination=-1;
        }
        if(tar.isTypeJ()||tar.type==Jalr){
            //如果是跳转指令，则可以立刻知道其运算结果，都不用放在RS里
            new_rob_e.rob_state=ReorderBufferElements::write;
            new_rob_e.value=tar.pc+4;
        }
        if(tar.type==Li){
            new_rob_e.rob_state=ReorderBufferElements::special;
        }
        elements.push_back(new_rob_e);
        return new_rob_e.entry;
    }
    CommitMessage try_commit_top(){
        CommitMessage ans;
        if((!elements.empty())&&(elements.front().rob_state==ReorderBufferElements::write||elements.front().rob_state==ReorderBufferElements::special))
            return commit(elements.front());

        return ans;
    }
    //
    int cnt=0;
    //
    CommitMessage commit(ReorderBufferElements& tar){
        //注意commit要放在统计write之后，防止没被统计就交上去的情况;
        ++cnt;
        if(cnt){
           for(int i=0;i<32;++i){
                cout<<(int)(cr->r[i])<<' ';
            }
           cout<<'\n';
        }

        //cout<<tar.ins.pc<<' '<<tar.value<<' '<<tar.destination<<'\n';
        if(tar.rob_state==ReorderBufferElements::special){
            std::cout<<((cr->r[10])&0xff);
            exit(0);
        }
        CommitMessage ans;
        if(tar.ins.isCalc()||tar.ins.isTypeI()||tar.ins.isTypeJ()||tar.ins.isTypeU()){
            ans.rob_e=tar;
            ans.v=tar.value;
            ans.register_change = true;
            ans.destination=tar.destination;
        }
        if(tar.ins.isTypeS()){
            ans.rob_e=tar;
            ans.v=tar.value;
            ans.memory_change = true;
            ans.address=tar.tar_address;

        }
        if(tar.ins.isTypeB()){

            ans.rob_e=tar;
            pre->busy=false;
            if(pre->predict_jump!=tar.value){
                //需要冲刷流水线
                pre->should_jump=(bool)tar.value;
                ans.flush_flag=true;
                if(pre->should_jump){
                    if(pre->a<3) ++pre->a;
                }else{
                    if(pre->a>0) --pre->a;
                }
            }else{
                if(pre->predict_jump){
                    if(pre->a<3) ++pre->a;
                }else{
                    if(pre->a>0) --pre->a;
                }
            }
        }
        tar.rob_state=ReorderBufferElements::commit;
        tar.busy= false;
        elements.pop_front();//被commit的一定是第一个
        return ans;
    }
    bool empty(){
        return elements.empty();
    }

    message_from_reorder_buffer run(){
        message_from_reorder_buffer ans;
        for(int i=elements.begin_num();(i)%32!=elements.end_num();++i){
            if(i>=32) i%=32;
            if(elements[i].busy&&elements[i].rob_state==ReorderBufferElements::write){
                ans.written_entry.push_back(i);
                ans.written_value.push_back(elements[i].value);
            }
        }
        ans.cm=try_commit_top();
        return ans;
    }
    void flush(){
        elements.clear();
    }
};
#endif //MAIN_CPP_REORDERBUFFER_HPP
