CC=gcc
CFLAGS=-Wall -pthread

all: QRServer QRClient QRMain

QRServer: server.o
	$(CC) -o $@ $^ $(CFLAGS)

QRClient: client.o
	$(CC) -o $@ $^ $(CFLAGS)

QRMain: main.o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o QRServer QRClient QRMain

.PHONY: clean all
