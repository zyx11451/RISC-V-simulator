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
#include "alu.hpp"
//为让每个周期各元件能乱序执行，必须记录上个周期的状态。
class ControlUnit{
    long long clock_cycle;
    memory ComputerMemory;
    InstructionUnit ComputerInstructionUnit;
    ALU computer_alu[8];//0到3号，用于一般指令计算，4,5用于分支计算，6,7内存地址的计算
};
#endif //MAIN_CPP_COMPUTER_HPP
