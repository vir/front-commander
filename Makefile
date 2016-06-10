# Use "gmake" on BSD and "make" on Linux

CXXFLAGS=-g -std=c++11 -Wall -pipe -pedantic
ifneq (,$(wildcard /usr/include/readline/readline.h))
CXXFLAGS+=-DHAVE_READLINE
LDFLAGS+=-lreadline
endif

all: game

.SUFFIXES: .cxx .so

.cxx.o:
	g++ $(CXXFLAGS) -c -o $*.o $<

game: game.o
	g++ $(LDFLAGS) -o $@ $^

#---------------------------------------------------------------------------
.PHONY: clean
clean:
	-rm -f *.o .depend core

tgz: clean
	rm -f ${TARNAME}
	tar czf --exclude '*.o' ${TARNAME} *

.depend:
	gcc -MM -MG *.c *.cxx >.depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

# vim: ft=make


