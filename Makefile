CC=cc
OUT=qsp
CFLAGS=-std=c99 -Wall -g
CLIBS=-ledit -lm

all: qsp

qsp: main.o lval.o mpc.o hmap.o builtins.o
	$(CC) $(CFLAGS) main.o lval.o mpc.o hmap.o builtins.o $(CLIBS) -o qsp

main.o: main.c lval.o mpc.o
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<

lval.o: lval.c lval.h hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<
	
builtins.o: builtins.c lval.h hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<
	
hmap.o: hmap.c hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<

mpc.o: mpc.c mpc.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<

clean:
	rm -rf *.o $(OUT)