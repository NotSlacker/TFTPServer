CFILES = src/packet.c src/client.c src/transfer.c src/server.c
FLAGS = -Wall -Wextra

default: all

all: server

server:
	${CC} ${FLAGS} -o tftps ${CFILES}

clean:
	rm tftps

test/all: test/small test/medium test/large

test/small:
	dd if=/dev/random of=$@ bs=1M count=1

test/medium:
	dd if=/dev/random of=$@ bs=1M count=5

test/large:
	dd if=/dev/random of=$@ bs=1M count=15