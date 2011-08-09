#!/usr/bin/tclsh
load ./sniplet.so
array set digit {
    0 "zero"
    1 "one"
    2 "two"
}
puts [ array get digit ]
set ret [ lazy {digit {index 2}} {
    puts -nonewline "lazy "
    puts [array get digit]
    return $digit($index)
} ]
puts "stady"
puts "1=$ret"
