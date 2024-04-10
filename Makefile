CFLAGS = -g -std=c99 -Wall -I. -I/usr/local/include 

LDFLAGS = -g -L/usr/local/include
LDLIBS = -lraylib -lm -ldl -lmysqlclient -lpthread -lrt -latomic -lssl -lcrypto

build:
	clang $(CFLAGS) $(LDFLAGS) main.c -o main.o $(LDLIBS) 

run:
	./main.o

debug: 
	rm -rf ./main.o
	clang $(CFLAGS) $(LDFLAGS) main.c -o main.o $(LDLIBS)
	echo `clear`
	gdb ./main.o
	

clean:
	echo `clear`
	rm -rf ./main.o

all: clean build run
		
	