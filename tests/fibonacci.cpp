#include <functional>
#include <iostream>


int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 0;
    }

    std::function<int64_t(int64_t)> fib = [&fib](int64_t n) {
        if (n == 0 || n == 1) {
            return n;
        } else {
            return fib(n - 2) + fib(n - 1);
        }
    };

    std::cout << "fibonacci: " << fib(atoi(argv[1])) << std::endl;
    return 0;
}
