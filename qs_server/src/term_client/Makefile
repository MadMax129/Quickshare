CC = gcc
CFLAGS = -I"../../../shared" -lpthread -lncurses
LDFLAGS = -lcrypto -lssl

all: quickshare

quickshare: t_client.c ../../../shared/msg.h
	$(CC) $^ -o $@ $(LDFLAGS) $(CFLAGS)

clean:
	rm -f quickshare