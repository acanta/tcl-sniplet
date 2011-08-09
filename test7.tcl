#!/usr/bin/tclsh
load ./sniplet.so
set a a
sniplet a {
    puts -nonewline "Locals: "
    puts [ info locals ]
    puts -nonewline "Args: "
    puts -nonewline [ info args ]
}
