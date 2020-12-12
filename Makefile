CXX = gcc

cfiles = src/packet.c src/transfer.c src/server.c
flags = -Wall

default: all

all: server

server:
	${CXX} ${flags} -o tftps ${cfiles}

clean:
	rm tftps
