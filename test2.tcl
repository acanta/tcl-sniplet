#!/usr/bin/tclsh
load ./sniplet.so
set a 1
set b [ sniplet { a } { 
    puts "!!!!!!!!!!!!!!!!!!!"
    puts [ info locals ] 
    set a 2
} ]
puts "$a $b"
# should be "a\n1 2"
