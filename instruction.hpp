//
// Created by 123 on 2023/6/26.
//

#ifndef MAIN_CPP_INSTRUCTION_HPP
#define MAIN_CPP_INSTRUCTION_HPP
#include <iostream>
enum order_type{
    Lui,
    Auipc,
    Jal,
    Jalr,
    Beq,
    Bne,
    Blt,
    Bge,
    Bltu,
    Bgeu,
    Lb,
    Lh,
    Lw,
    Lbu,
    Lhu,
    Sb,
    Sh,
    Sw,
    Addi,
    Slti,
    Sltiu,
    Xori,
    Ori,
    Andi,
    Slli,
    Srli,
    Srai,
    Add,
    Sub,
    Sll,
    Slt,
    Sltu,
    Xor,
    Srl,
    Sra,
    Or,
    And,
    Null,
    Error,
    Li
};
class instruction{
private:

public:
    //可能需要记一下在保留站中位置
    unsigned int instruction_ID;//用于区分不同指令，随指令条数递增
    order_type type;
    int rd;
    uint32_t immediate;//存储算完后的地址
    uint32_t rs1,rs2;
    uint32_t offset;
    uint32_t pc;//当前指令在内存中的位置
    instruction(){
        instruction_ID=0;
        type=order_type::Null;
        rd=0;
        immediate=0;
        rs1=0;
        rs2=0;
        offset=0;
    }
    bool isTypeU() const{
        return type==Lui||type==Auipc;
    }
    bool isTypeI() const{
        return type==Jalr||type==Lb||type==Lh||type==Lw||type==Lbu||type==Lhu
        ||type==Addi||type==Slti||type==Sltiu||type==Xori||type==Ori||type==Andi
        ||type==Slli||type==Srli||type==Srai;
    }
    bool isCalc() const{
        return type==Addi||type==Slti||type==Sltiu||type==Xori||type==Ori||type==Andi
               ||type==Slli||type==Srli||type==Srai||type==Add||type==Sub||type==Sll||
               type==Slt||type==Sltu||type==Xor||type==Srl||type==Sra||type==Or||type==And;
    }
    bool isTypeS() const{
        return type==Sb||type==Sh||type==Sw;
    }
    bool isTypeB() const{
        return type==Beq||type==Bne||type==Blt||type==Bge||type==Bltu||type==Bgeu;
    }
    bool isTypeJ() const{
        return type==Jal;
    }
    bool isTypeR() const{
        return type==Add||type==Sub||type==Sll||type==Slt||type==Sltu||type==Xor
        ||type==Srl||type==Sra||type==Or||type==And;
    }
    bool isLoad() const{
        return type==Lb||type==Lh||type==Lw||type==Lbu||type==Lhu;
    }
};
#endif //MAIN_CPP_INSTRUCTION_HPP
