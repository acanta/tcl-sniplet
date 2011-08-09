#!/usr/bin/tclsh
load ./sniplet.so
set a 1
set b [ sniplet { set a 2 } ]
puts $a
puts $b
# should be "1\n2\n"


