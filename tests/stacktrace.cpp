#include <print>
#include <stacktrace>

void trace()
{
    std::print("{}", std::stacktrace::current());
}

int main()
{
    trace();
    return 0;
}

// CompileOptions: -lstdc++exp
