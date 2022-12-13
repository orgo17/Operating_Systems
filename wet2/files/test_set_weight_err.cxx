#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>


int main() {
    int ret = get_weight();
    assert(ret == 0);
    ret = set_weight(-1);
    assert(errno == EINVAL);
    ret = set_weight(-100);
    assert(errno == EINVAL);
    ret = set_weight(-5646546);
    assert(errno == EINVAL);
    ret = get_weight();
    assert(ret == 0);
    std::cout << "===== SUCCESS =====" << std::endl;
    return 0;
}

