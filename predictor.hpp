//
// Created by 123 on 2023/7/1.
//

#ifndef MAIN_CPP_PREDICTOR_HPP
#define MAIN_CPP_PREDICTOR_HPP
#include <iostream>
struct Predictor_message{
    bool flush_flag=false;
    uint32_t target_pc;//如果需要重新跳转，则跳转目标是
};
class Predictor{
    //二位饱和分支预测器
public:
    int a=0;//0,1,2,3
    bool busy=false;//是否被占用，这也意味着是否有B指令已发射但未提交
    bool predict_jump= false;
    uint32_t now_pc;//不跳转时位置
    uint32_t jump_pc;//跳转后位置
    bool should_jump;
};
#endif //MAIN_CPP_PREDICTOR_HPP
