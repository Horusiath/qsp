CC=cc
BIN=./bin/
SRC=./src/
OUT=$(BIN)qsp
CFLAGS=-std=c99 -Wall -g
CLIBS=-ledit -lm
COMP=$(BIN)main.o $(BIN)lval.o $(BIN)mpc.o $(BIN)hmap.o $(BIN)builtins.o $(BIN)gc.o

all: $(BIN)qsp

$(BIN)qsp: $(COMP)
	$(CC) $(CFLAGS) $(COMP) $(CLIBS) -o $(BIN)qsp

$(BIN)main.o: $(SRC)main.c $(BIN)lval.o $(BIN)mpc.o
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<

$(BIN)lval.o: $(SRC)rt/lval.c $(SRC)rt/lval.h $(SRC)rt/hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<
	
$(BIN)builtins.o: $(SRC)rt/builtins.c $(SRC)rt/lval.h $(SRC)rt/hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<
	
$(BIN)gc.o: $(SRC)rt/gc.c $(SRC)rt/lval.h $(SRC)rt/hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<
	
$(BIN)hmap.o: $(SRC)rt/hmap.c $(SRC)rt/hmap.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<

$(BIN)mpc.o: $(SRC)proto/mpc.c $(SRC)proto/mpc.h
	$(CC) $(CFLAGS) $(CLIBS) -c -o $@ $<

clean:
	rm -rf $(BIN)*.o $(OUT)