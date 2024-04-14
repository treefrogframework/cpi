#include <iostream>
#include <type_traits>

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<Arithmetic T>
T add(T a, T b)
{
    return a + b;
}

int main()
{
    int a = 5, b = 3;
    float c = 2.5f, d = 4.4f;
    double e = 1.5, f = 2.3;

    std::cout << "add(int, int): " << add(a, b) << std::endl;       // 8
    std::cout << "add(float, float): " << add(c, d) << std::endl;   // 7.0
    std::cout << "add(double, double): " << add(e, f) << std::endl; // 3.8
    return 0;
}

