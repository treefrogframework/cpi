#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    int cnt = 5;
    if (argc > 1) {
        cnt = atoi(argv[1]);
    }

    for (int i = 0; i < cnt; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << i + 1 << ' ' << std::flush;
    }
    std::cout << std::endl;
    return 0;
}
