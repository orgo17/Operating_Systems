#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <wait.h>


int main() { // A
    pid_t A = getpid();
    set_weight(10);
    pid_t ret = get_heaviest_ancestor();
    assert(ret == getpid());
    if(fork() == 0) { // B 
        set_weight(5);
        pid_t B_parent = getppid(); // A pid
        ret = get_heaviest_ancestor();
        assert(ret == B_parent);
        if(fork() == 0) { // C
            set_weight(1);
            pid_t C_parent = getppid(); // B pid
            ret = get_heaviest_ancestor();
            assert(ret == B_parent);
            if(fork() == 0) { // D
                set_weight(25);
                pid_t D_parent = getppid(); // C pid
                ret = get_heaviest_ancestor();
                assert(ret == getpid());
                if(fork() == 0) { // E
                    set_weight(10);
                    pid_t E_parent = getppid(); // D pid
                    pid_t ret = get_heaviest_ancestor();
                    assert(ret == D_parent);
                }
                else{
                    wait(nullptr);
                }
            }
            else{
                wait(nullptr);
            }
        }
        else{
            wait(nullptr);
        }
    }
    else{
        wait(nullptr);
        std::cout << "===== SUCCESS =====" << std::endl;
    }    
    return 0;
}