#include <generator>
#include <iostream>


std::generator<uint64_t> fib(int n)
{
    uint64_t a = 0, b = 1;

    for (int i = 0; i <= n; ++i) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 0;
    }

    uint64_t num = 0;
    for (auto v : fib(atoi(argv[1]))) {
        num = v;
    }

    std::cout << "fibonacci: " << num << std::endl;
    return 0;
}
