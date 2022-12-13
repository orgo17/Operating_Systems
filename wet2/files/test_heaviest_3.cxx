#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <wait.h>


int main() { // A
    pid_t A = getpid();
    if(fork() == 0) { // B 
        pid_t B_parent = getppid(); // A pid
        if(fork() == 0) { // C
            pid_t C_parent = getppid(); // B pid
            if(fork() == 0) { // D
                pid_t D_parent = getppid(); // C pid
                if(fork() == 0) { // E
                    pid_t E_parent = getppid(); // D pid
                    pid_t ret = get_heaviest_ancestor();
                    assert(ret == getpid());
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