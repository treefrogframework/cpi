# Tiny C++ Interpreter

Cpi is a tiny interpreter for C++11 code.

## Requirements
 - Qt qmake build system
 - Compiler - g++, clang++, Visual C++ compiler

## Install

    $ qmake
    $ make
    $ sudo make install


## Run Mode
Save C++ source code as 'hello.cpp'.

    int main()
    {
        std::cout << "Hello world\n";
        return 0;
    }


Run cpi in command line.

    $ cpi hello.cpp
    Hello world

Immediately compiled and executed!

Specify options for compiler or linker with "CompileOptions: " word.

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

In this example, math library specified by "-lm" option.

    $ cpi sqrt.cpp 2
    1.41421

    $ cpi sqrt.cpp 3
    1.7320

## Interactive Mode

    $ cpi
    Cpi 2.0.0
    Type ".help" for more information.
    Loaded INI file: /home/foo/.config/cpi/cpi.conf

    cpi>  3 << 23;        (Bitwise operation)
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

## Help

    cpi> .help
     .conf        Display the current values for various settings.
     .help        Display this help.
     .rm LINENO   Remove the code of the specified line number.
     .show        Show the current source code.
     .quit        Exit this program.

## Download
 [Download Page](https://github.com/treefrogframework/cpi/releases)


## Web Site
 http://treefrogframework.github.io/cpi/

