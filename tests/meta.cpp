#include <meta>
#include <print>

struct User {
    int id;
    const char* name;
};

int main()
{
    // static reflection
    constexpr auto clsinfo = ^^User;
    constexpr auto ctx = std::meta::access_context::current();

    std::println("Class: {}", std::meta::identifier_of(clsinfo));

    template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(clsinfo, ctx))) {
        std::println("  Member: {}  ({})", std::meta::identifier_of(m), std::meta::display_string_of(std::meta::type_of(m)));
    }

    // splice operator
    User user{1954, "Godzilla"};
    constexpr auto info = ^^user;

    std::println("\nVariable: {}", std::meta::identifier_of(info));

    template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(clsinfo, ctx))) {
        std::println("  {} = {}", std::meta::identifier_of(m), user.[:m:]);
    }

    return 0;
}

// CompileOptions: -freflection
