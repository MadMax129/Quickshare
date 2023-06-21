# Quickshare Server Linux Makefile
CXX = gcc
EXE = qs_s
BUILD_DIR = build

SRC = \
	queue.c       \
	database.c    \
	secure.c      \
	client.c      \
	net_close.c   \
	net_write.c   \
	net_packet.c  \
	net_read.c    \
	network.c     \
	qs_server.c

SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRC))))
SRC_OBJS += $(MEM_POOL_DIR)/mem.o $(UTIL_DIR)/util.o

# Shared Libs
MEM_POOL_DIR = ../../shared/mem_pool
UTIL_DIR     = ../../shared/util
SHARED_DIR   = ../../shared

SRC_INC_DIR =         \
	-I$(MEM_POOL_DIR) \
	-I$(UTIL_DIR)     \
	-I$(SHARED_DIR)   \
	-I../include      
DEPS = $(addprefix build/, $(SRC:.c=.d))

CFLAGS += 	-O2 -Wall -Werror -Wextra                             \
			-std=c11 -pedantic-errors -Wno-error=unused-parameter \
			-Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings \
			-Wcast-qual -Wswitch-enum -Wunreachable-code
LIBS = -lpthread -lsqlite3 -lssl -lcrypto -luv

PRIVATE_KEY = "./cert/server.key"
SERVER_CERT = "./cert/server.crt"

.PHONY: all clean tls_cert

all: dir $(EXE)
	@echo Build Complete
	
dir:
	@mkdir -p build

tls_cert:
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

# Build Memory Pool
$(MEM_POOL_DIR)/mem_pool.o:
	$(MAKE) -C $(MEM_POOL_DIR)

# Build Util
$(UTIL_DIR)/util.o:
	$(MAKE) -C $(UTIL_DIR)

# Build App Source
$(BUILD_DIR)/%.o:%.c
	$(CXX) $(CFLAGS) $(SRC_INC_DIR) -MMD -MP -c -o $@ $<

# Link Together
$(EXE): $(SRC_OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(CFLAGS)

-include $(DEPS)