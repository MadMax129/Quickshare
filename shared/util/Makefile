CC     = gcc
TARGET = util.o
SRC    = util.c
HDR    = util.h

CFLAGS += -O2 -Wall -Werror -Wextra \
		  -std=c11 -pedantic-errors -Wno-error=unused-parameter \
		  -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings \
		  -Wcast-qual -Wswitch-enum -Wunreachable-code
LDFLAGS = 

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(TARGET)
