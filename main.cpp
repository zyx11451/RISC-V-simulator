#include <iostream>
#include "Computer.hpp"
int main() {
    freopen("a.in","r",stdin);
    freopen("a.out","w",stdout);
    ControlUnit my_simulator;
    my_simulator.ini();
    while (true){
        my_simulator.run();
    }
}
