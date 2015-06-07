# C++ Interpreter

## Requirements
 - Qt qmake build system.

## Install

    $ qmake
    $ make
    $ sudo make install

## Run

    $ cpi
    Loaded INI file: /home/foo/.config/cpi/cpi.conf
    cpi> int a;
    cpi> a = 2 << 5;
    64
    
    cpi> .quit

## Help

    $ cpi
    Loaded INI file: /home/foo/.config/cpi/cpi.conf
    cpi> .help
     .conf        Display the current values for various settings.
     .help        Display this help.
     .rm LINENO   Remove the code of the specified line number.
     .show        Show the current source code.
     .quit        Exit this program.

