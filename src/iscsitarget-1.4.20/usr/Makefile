CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes -g -I../include
CFLAGS += -D_GNU_SOURCE # required for glibc >= 2.8
PROGRAMS = ietd ietadm
LIBS =

all: $(PROGRAMS)

ietd: ietd.o iscsid.o conn.o session.o target.o message.o ctldev.o log.o chap.o event.o param.o plain.o isns.o md5.o sha1.o

	$(CC) $^ -o $@ $(LIBS)

ietadm: ietadm.o param.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGRAMS)
