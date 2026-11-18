#include <vector>
#include <string>
#include <ranges>
#include <print>

int main() {
    auto names = {"Alice", "Bob", "Charlie", "Dan"};
    auto ages = {24, 50, 18};

    for (auto&& [name, age] : std::views::zip(names, ages)) {
	std::println("{}: {}", name, age);
    }

    return 0;
}
