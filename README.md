# C++ Interpreter

## Requirements
 - Qt qmake build system
 - g++ compiler

## Install

    $ qmake
    $ make
    $ sudo make install

## Run

    $ cpi
    Loaded INI file: /home/foo/.config/cpi/cpi.conf
    cpi>  3 << 23;        (Bitwise operation)
    25165824
    
    cpi> int a = 3;
    cpi> ~a;              (Complement)
    -4
    cpi> a ^ 2;           (XOR)
    1
    
    cpi> .quit

## Help

    cpi> .help
     .conf        Display the current values for various settings.
     .help        Display this help.
     .rm LINENO   Remove the code of the specified line number.
     .show        Show the current source code.
     .quit        Exit this program.

## Download
 [Download Page](https://github.com/treefrogframework/cpi/releases)
