import std;

// delete here if supported by stdlib
template <>
struct std::formatter<std::u8string, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::u8string &str, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
    }
};

std::optional<int> get_even_number(bool get)
{
    if (get) return 2;
    return std::nullopt;
}

int main()
{
    std::u8string msg = u8"Hello, Hola, Olá, 你好, こんにちは";
    std::println("{}", msg);

    auto num = get_even_number(true);
    std::println("Number is {}", (num ? std::to_string(num.value()) : "not present"));

    return 0;
}

// CompileOptions: -fmodules --compile-std-module
