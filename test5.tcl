#!/usr/lib/tclsh
load ./sniplet.so
set a [ lazy { puts "execute" ; return "from lazy" } ]
puts "stady"
puts $a
# should be "stady\n execute\n from lazy"
