#!/usr/bin/tclsh
load ./sniplet.so
sniplet { set uniqueName 11 }
puts [ info vars unique* ]
# should be nothing
