CXX = gcc
EXE = qs_server

SOURCES = 	./memory/memory.c \
			server.c
SOURCE_HDR = $(SOURCES:.c=.h)
CXXFLAGS += -O2 -Wall -std=c11 -pedantic-errors
LIBS = -lpthread

.PHONY: all clean

all: $(EXE)
	@echo "Build Complete!"

clean:
	@echo "Cleaning"
	rm $(EXE)

$(EXE): $(SOURCES) $(SOURCE_HDR)
	$(CXX) -o $(EXE) $(SOURCES) $(CXXFLAGS) $(LIBS)