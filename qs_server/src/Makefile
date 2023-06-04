# QS Server Makefile

CXX = gcc
EXE = qs_s
BUILD_DIR = build

SRC = \
	ringbuffer.c \
	secure.c      \
	client.c      \
	network.c     \
	qs_server.c
SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRC))))

RING_BUF_DIR = ../lib/Ring-Buffer

SRC_INC_DIR =         \
	-I$(RING_BUF_DIR) \
	-I../include      \
	-I../../include
DEPS = $(addprefix build/, $(SRC:.c=.d))

CXXFLAGS += -O2 -Wall -Werror -Wextra \
			-std=c11 -pedantic-errors -Wno-error=unused-parameter
LIBS = -lpthread -lsqlite3 -lssl -lcrypto -luv

# ADD OS CHECK

.PHONY: all clean install_deps

all: dir $(EXE)
	@echo Complete

install_deps: install_asio

install_asio:
	@mkdir -p ../lib
	cd ../lib && git clone https://github.com/chriskohlhoff/asio.git
	
dir:
	@mkdir -p build

clean:
	rm -f -r $(BUILD_DIR)
	rm -f $(EXE)

# Build Ring-Buffer
$(BUILD_DIR)/ringbuffer.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $(RING_BUF_DIR)/ringbuffer.c

# Build App Source
$(BUILD_DIR)/%.o:%.c
	$(CXX) $(CXXFLAGS) $(SRC_INC_DIR) -MMD -MP -c -o $@ $<

# Link Together
$(EXE): $(SRC_OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(CXXFLAGS)

-include $(DEPS)