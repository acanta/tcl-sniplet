TCL_INCLUDE=-I/usr/include/tcl8.6/generic

TCL_CONFIG=/usr/lib/tclConfig.sh
STUB=/usr/lib/libtclstub8.6.a
CFLAGS= -std=c99 -Wall -Wextra $(TCL_INCLUDE) -DUSE_TCL_STUBS -DNDEBUG
LDLIBS=tcl8.6

sniplet.so: sniplet.o
	gcc -fPIC -shared -l$(LDLIBS) $? $(STUB) -o $@

#sniplet.o : sniplet.c

.PHONY: clean touch vars

clean:
	rm -f sniplet.so
	rm -f *.o
	rm -f *~

touch:
	touch -c *.c
	touch -c *.h
	touch Makefile

vars:
	@echo "CFLAGS=$(CFLAGS)"

