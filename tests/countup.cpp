#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    int cnt = 5;
    std::cout << "How many times will it count up?" << std::endl;
    std::cin >> cnt;

    for (int i = 0; i < cnt; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << i + 1 << ' ' << std::flush;
    }
    std::cout << std::endl;
    return 0;
}
