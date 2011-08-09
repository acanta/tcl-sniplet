#!/usr/bin/tclsh
load ./sniplet.so
puts [ info commands sniplet ]
puts [ info commands lazy ]
set a [ sniplet { return "hello" } ]
puts $a

