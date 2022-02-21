#include <cmath>
#include <cstdlib>
#include <iostream>


int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 0;
    }

    auto square = [](int n) { return n * n; };  // lambda expressions

    std::cout << "sqrt: " << sqrt(atoi(argv[1])) << std::endl;
    std::cout << "square: " << square(atoi(argv[1])) << std::endl;
    return 0;
}

// CompileOptions: -lm
