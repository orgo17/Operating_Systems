#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <wait.h>


int main() { // A
    get_leaf_children_sum();
    assert(errno == ECHILD);
    std::cout << "===== SUCCESS =====" << std::endl;
    return 0;
}