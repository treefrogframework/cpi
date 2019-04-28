# Tiny C++ Interpreter

Cpi is a tiny interpreter for C++11 or C++14.

## Requirements
  * Qt qmake build system
  * Compiler - GNU C++ compiler or LLVM C++ compiler

## Install

```sh
  $ qmake
  $ make
  $ sudo make install
```

## Executive mode
Save C++ source code as *hello.cpp*.

```cpp
#include <iostream>

int main()
{
    std::cout << "Hello world\n";
    return 0;
}
```

Run cpi in command line.

```sh
  $ cpi hello.cpp
  Hello world
```

Immediately compiled and executed! Almost a script language, but the source file is also C++ program which a compiler can compile successfully.  

Next code outputs a square root of input argument.  
Specify options for compiler or linker with "CompileOptions: " word. In this example, linking math library specified by "-lm" option.

```cpp
#include <iostream>
#include <cmath>
#include <cstdlib>
using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 2) return 0;

    cout << sqrt(atoi(argv[1])) << endl;
    return 0;
}
// CompileOptions: -lm
```

```sh
  $ cpi sqrt.cpp 2
  1.41421

  $ cpi sqrt.cpp 3
  1.7320
```

#### Running like a scripting language
Adding a shebang, save as *hello.cpps*. No longer compiled in a C++ compiler successfully.

```cpp
#!/usr/bin/env cpi
#include <iostream>

int main()
{
    std::cout << "Hello world\n";
    return 0;
}
```

```sh
  $ chmod +x hello.cpps
  $ ./hello.cpps
  Hello world
```

Yes, a shell script. I named it CppScript.


## Interactive Mode

```
  $ cpi
  Cpi 2.0.0
  Type ".help" for more information.
  Loaded INI file: /home/foo/.config/cpi/cpi.conf

  cpi> 3 << 23;        (Bitwise operation)
  25165824
  
  cpi> int a = 3;
  cpi> ~a;              (Complement)
  -4
  cpi> a ^ 2;           (XOR)
  1
  
  cpi> auto func = [](int n) { return n*n; };     (Lambda function)
  cpi> func(3);
  9
    
  cpi> .quit         ( or press ctrl+c )
```

## Help

```
  cpi> .help
   .conf        Display the current values for various settings.
   .help        Display this help.
   .rm LINENO   Remove the code of the specified line number.
   .show        Show the current source code.
   .quit        Exit this program.
```

## Download
 [Download Page](https://github.com/treefrogframework/cpi/releases)


## Web Site
 http://treefrogframework.github.io/cpi/
