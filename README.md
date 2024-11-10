# Tiny C++ Interpreter

[![ActionsCI](https://github.com/treefrogframework/cpi/actions/workflows/actions.yml/badge.svg)](https://github.com/treefrogframework/cpi/actions/workflows/actions.yml)
[![License](https://img.shields.io/badge/license-MIT-blue)](https://opensource.org/licenses/MIT)
[![Release](https://img.shields.io/github/v/release/treefrogframework/cpi.svg)](https://github.com/treefrogframework/cpi/releases)

Cpi is a tiny interpreter for C++17 or C++20.

## Requirements
The following softwares are needed to build and execute cpi.
The compiler is used as interpreter of cpi internally.
  * Qt tookit version 6
  * Compiler - GNU C++ compiler, LLVM C++ compiler or MSVC C++ compiler

## Install

Linux:
```sh
$ qmake
$ make
$ sudo make install
$ cpi -v
cpi 2.2.0
```

Windows (Command prompt for VS2022):
```bat
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.9.6
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'x64'

> C:\Qt\6.7.0\msvc2019_64\bin\qtenv2.bat
Setting up environment for Qt usage...
> cd (cpi root directory)
> qmake
> nmake
> cpi.bat -h        (Run cpi command)
Usage: cpi.exe [options] [file] [-]
Tiny C++ Interpreter.
Runs in interactive mode by default.

Options:
  -?, -h, --help  Displays help on commandline options.
  --help-all      Displays help, including generic Qt options.
  -v, --version   Displays version information.

Arguments:
  file            File to compile.
  -               Reads from stdin.
```

## Interactive Mode

```
  $ cpi           (Run cpi.bat in windows)
  cpi 2.2.0
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

Code can be pasted.
```cpp
  $ cpi              (Run cpi.bat in windows)
  cpi> #include <map>         (Paste code here)
  #include <iostream>

  int main()
  {
    std::map<int, std::string> m = { {1, "one"}, {2, "two"} };
    if (auto it = m.find(2); it != m.end()) {
      std::cout << it->second << std::endl;
    }
  }              (Press enter)
  two            (The result of the executed output)
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

int main(int argc, char *argv[])
{
    if (argc != 2) return 0;

    std::cout << sqrt(atoi(argv[1])) << std::endl;
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

Furthermore, pkg-config command can be used for *CompileOptions*.

```cpp
#include <cblas.h>
#include <iostream>

int main()
{
    // 2x2 Matrix
    int M = 2, N = 2, K = 2;
    double A[4] = {1.0, 2.0, 3.0, 4.0};
    double B[4] = {5.0, 6.0, 7.0, 8.0};
    double C[4];

    // General Matrix-Matrix multiplication
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
        M, N, K,
        1.0, A, K, B, N,
        0.0, C, N);

    // Print results
    std::cout << "Result of A * B = C:" << std::endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            std::cout << C[i * 2 + j] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}

// CompileOptions: `pkg-config --cflags --libs openblas`
```

```sh
 $ cpi dgemm.cpp
 Result of A * B = C:
 19 22
 43 50
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
