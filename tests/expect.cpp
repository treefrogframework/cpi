#include <expected>
#include <string>
#include <print>

std::expected<long long, std::string> parseNumber(const std::string &s)
{
    try {
        return std::stoll(s);
    } catch (...) {
        return std::unexpected("invalid number");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 1;
    }

    auto ret = parseNumber(std::string(argv[1]));

    if (ret) {
        auto n = *ret;
        std::println("Number is {}", n);
    } else {
        std::println("Error: {}", ret.error());
    }
    return 0;
}
