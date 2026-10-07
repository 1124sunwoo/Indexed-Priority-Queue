#include "IndPQ.h"
#include <iostream>

int main() {
    IndPQ tasks;
    tasks.insert("write-report", 3);
    tasks.insert("fix-bug", 1);
    tasks.insert("review-code", 2);
    tasks.updatePriority("write-report", 0);
    tasks.remove("review-code");
    while (!tasks.isEmpty()) std::cout << tasks.deleteMin() << '\n';
}
