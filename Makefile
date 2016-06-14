# Use "gmake" on BSD and "make" on Linux

CXXFLAGS=-g -std=c++11 -Wall -pipe -pedantic
ifneq (,$(wildcard /usr/include/readline/readline.h))
CXXFLAGS+=-DHAVE_READLINE
LDFLAGS+=-lreadline
endif
ifneq (,$(wildcard /usr/include/boost/locale.hpp))
CXXFLAGS+=-DHAVE_BOOST_LOCALE
LDFLAGS+=-lboost_locale
endif
ifneq (,$(wildcard /usr/include/boost/filesystem.hpp))
CXXFLAGS+=-DHAVE_BOOST_FILESYSTEM
LDFLAGS+=-lboost_filesystem -lboost_system
endif

all: game

.SUFFIXES: .cxx

.cxx.o:
	g++ $(CXXFLAGS) -c -o $*.o $<

game: game.o
	g++ $(LDFLAGS) -o $@ $^

#---------------------------------------------------------------------------
.PHONY: clean
clean:
	-rm -f *.o core

# vim: ft=make


