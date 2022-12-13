#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <wait.h>


int main() { // A
    pid_t B,C;
    if((B = fork()) == 0) { // B child
        set_weight(1);
        while(true);
    }
    else {
        if((C = fork()) == 0) { // C child
            if(fork() == 0) { // D child
                set_weight(100);
                sleep(1000);
            }
            else {
                set_weight(10);
                while(true);
            }
        }
        else { // A
            sleep(3);
            int ret = get_leaf_children_sum();
            assert(ret == 101);
            kill(B, SIGKILL);
            waitpid(B, nullptr, 0);
            ret = get_leaf_children_sum();
            assert(ret == 100);
            kill(C, SIGKILL);
            waitpid(C, nullptr, 0);
            ret = get_leaf_children_sum();
            assert(errno == ECHILD);
            std::cout << "===== SUCCESS =====" << std::endl;
        }
    }
    return 0;
}