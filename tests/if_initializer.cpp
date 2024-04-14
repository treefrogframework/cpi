#include <iostream>
#include <map>

int main()
{
    std::map<std::string, int> m{{"one", 1}, {"two", 2}, {"three", 3}};
    
    if (auto it = m.find("two"); it != m.end()) {
        std::cout << "Found " << it->second << std::endl;  // Found 2
    }
    return 0;
}

