CC=gcc
CFLAGS=-Wall -g
LIBSRC=context.c rules.c schedule.c proximity.c usage.c 
LIBOBJS=context.o rules.o schedule.o proximity.o usage.o 
INCS=rules.h 
ARFLAGS=-rv
LIBRULES=librules.a


all: $(LIBRULES) 

lib: $(LIBRULES)

$(LIBRULES): $(LIBOBJS) 
	$(AR) $(ARFLAGS) $@ $(LIBOBJS)

$(LIBOBJS): $(LIBSRC)

clean:
	rm -f *.o $(BIN) *.a *~ 
	rm -f -r _obj *.dSYM 

