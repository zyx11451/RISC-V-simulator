//
// Created by 123 on 2023/6/25.
//

#ifndef MAIN_CPP_MY_QUEUE_HPP
#define MAIN_CPP_MY_QUEUE_HPP
namespace kingzyx{
    template<class T,int size>
    class circular_queue{
        T data[size];
        int begin;
        int end;
    public:
        circular_queue(){
            begin=0;
            end=0;
        }
        ~circular_queue()=default;
        bool empty() const {
            return  begin==end;
        }
        bool full() const {
            return end==begin-1||(begin==0&&end==size-1);
        }
        void push_back(const T &tar){
            data[end]=tar;
            end=(end+1)%size;
        }
        void pop_front(){
            begin=(begin+1)%size;
        }
        T& front(){
            return data[begin];
        }
        int begin_num(){
            return begin;
        }
        int end_num(){
            return end;
        }
        T& operator[](const int in){
            return data[in];
        }
        void clear(){
            end=begin;
        }
    };
}
#endif //MAIN_CPP_MY_QUEUE_HPP
