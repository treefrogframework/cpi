#include <iostream>
#include <vector>
#include <print>

int main() {
    std::vector<int> vec = {1, 2, 3};

    switch (auto size = vec.size(); size) {
        case 0:
            std::println("Empty");
            break;
        case 1:
            std::println("Only one");
            break;
        default:
            std::println("There are {}", size);
            break;
    }
    return 0;
}
