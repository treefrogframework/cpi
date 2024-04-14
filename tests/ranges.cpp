#include <iostream>
#include <ranges>
#include <vector>

int main()
{
    std::vector<int> nums {1, 2, 3, 4, 5, 6};
    auto even = nums | std::views::filter([](int n) { return n % 2 == 0; });
    for (int n : even) {
        std::cout << n << " ";  // 2 4 6
    }
    std::cout << std::endl;
    return 0;
}

