#include <iostream>
#include <memory>

class Data {
public:
    Data() { std::cout << "Data created\n"; }
    ~Data() { std::cout << "Data destroyed\n"; }
    void doSomething() { std::cout << "Doing something..\n"; }
};

int main()
{
    auto ptr = std::make_unique<Data>();
    ptr->doSomething();
    return 0;
}
