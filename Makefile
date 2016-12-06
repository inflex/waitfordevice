CFLAGS=-Wall -O2

.c.o:
	${CC} ${CFLAGS} -c $*.c

all:	waitfordevice

waitfordevice:	$(OBJS) waitfordevice.c
	${CC} ${CFLAGS} $(OBJS) waitfordevice.c -o waitfordevice

install: waitfordevice
	strip waitfordevice
	cp waitfordevice /usr/local/bin

clean:
	rm waitfordevice
