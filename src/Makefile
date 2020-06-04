PROGNAME=arbiterd
CC=gcc
CFLAGS=-Wall -O2 -g `pkg-config glib-2.0 --cflags` --std=c99
OBJS=arbiterd.o client-thread.o utility.o
LIBS=-lc -lipset -pthread `pkg-config glib-2.0 --libs`

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)

all: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LIBS)  -o $(PROGNAME)

clean:
	rm -f *.o
	rm -f $(PROGNAME)
	rm -f *~

install:
	cp $(PROGNAME) /usr/local/bin
