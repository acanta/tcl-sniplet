TCL_CONFIG=/usr/lib64/tclConfig.sh

include $(TCL_CONFIG)

CFLAGS= -std=c99 -Wall -Wextra -fPIC -DHAVE_UNISTD_H -DUSE_TCL_STUBS -DNDEBUG

sniplet.so: sniplet.o
	gcc -shared $(TCL_STUB_LIB_PATH) $? -o $@

.PHONY: clean touch 

clean:
	rm -f sniplet.so
	rm -f *.o
	rm -f *~

touch:
	touch -c *.c
	touch -c *.h
	touch Makefile


