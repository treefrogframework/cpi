#include <print>
#include <string>

int main()
{
    std::u8string msg = u8"Hello, Hola, Olá, 你好, こんにちは";
    std::println("{}", std::string(msg.begin(), msg.end()));

    // c++26
    //std::println("{}", msg);
    return 0;
}
