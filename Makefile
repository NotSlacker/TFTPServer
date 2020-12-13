cfiles = src/packet.c src/transfer.c src/server.c
flags = -Wall -Wextra

default: all

all: server

server:
	${CC} ${flags} -o tftps ${cfiles}

clean:
	rm tftps

test/all: test/small test/medium test/large

test/small:
	dd if=/dev/random of=$@ bs=1M count=1

test/medium:
	dd if=/dev/random of=$@ bs=1M count=5

test/large:
	dd if=/dev/random of=$@ bs=1M count=15