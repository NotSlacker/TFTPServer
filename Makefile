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
	dd if=/dev/random of=$@ bs=100 count=1 iflag=fullblock

test/medium:
	dd if=/dev/random of=$@ bs=1000 count=10 iflag=fullblock

test/large:
	dd if=/dev/random of=$@ bs=10000 count=100 iflag=fullblock