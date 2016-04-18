#
# Makefile for temporary PCI library 
# 
# by Darmawan Salihun <darmawan.salihun@gmail.com>
#


LIBRARY = libpci.a

AR 	= ar
CC      = gcc
STRIP	= strip
CFLAGS  = -O2 -g -Wall -Werror
STRIP_ARGS = -s

OBJS = access.o filter.o generic.o i386-ports.o 

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	$(AR) -r $(LIBRARY) $(OBJS)

clean:
	rm -f *.o *~ *.a

.PHONY: all clean

