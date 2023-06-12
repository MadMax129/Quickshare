# QS Server Makefile

CXX = gcc
EXE = qs_s
BUILD_DIR = build

SRC = \
	queue.c       \
	ringbuffer.c  \
	database.c    \
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

PRIVATE_KEY = "./cert/server.key"
SERVER_CERT = "./cert/server.crt"

# ADD OS CHECK

.PHONY: all clean install_deps tls

all: dir $(EXE)
	@echo Complete

install_deps: install_asio

install_asio:
	@mkdir -p ../lib
	cd ../lib && git clone https://github.com/chriskohlhoff/asio.git
	
dir:
	@mkdir -p build

tls:
	openssl genrsa \
		-des3 -passout pass:qsServer \
		-out ./cert/s.pass.key 2048
	
	openssl rsa \
		-passin pass:qsServer -in ./cert/s.pass.key \
		-out $(PRIVATE_KEY)
	rm -f ./cert/s.pass.key

	openssl req -new -key $(PRIVATE_KEY) -out ./cert/server.csr
	openssl x509 \
		-req -sha256 -days 365 -in \
		./cert/server.csr -signkey $(PRIVATE_KEY) \
		-out $(SERVER_CERT)
	rm -f server.csr

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