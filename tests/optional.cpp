#include <iostream>
#include <optional>

std::optional<int> get_even_number(bool get)
{
    if (get) return 2;
    return std::nullopt;
}

int main()
{
    auto num = get_even_number(false);
    std::cout << "Number is " << (num ? std::to_string(*num) : "not present") << std::endl; 
    return 0;
}

