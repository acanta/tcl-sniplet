#!/usr/bin/tclsh
load ./sniplet.so
set uniqueVar 66124
sniplet { set a $uniqueVar ; puts $a }
# should be a nothing