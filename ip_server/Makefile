# Ip_Server for hosting Quickshare server ip's

CXX = g++
EXE = qs_ip
BUILD_DIR = build

SRC = util.cpp        \
	  database.cpp    \
	  server.cpp      \
	  ip_server.cpp
SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRC))))
CXXFLAGS += -O2 -Wall -Werror -Wextra -std=c++17 -pedantic-errors -I./include -I../include
LIBS = -lpthread -lsqlite3

dir:
	@mkdir -p build

clean:
	rm -f -r $(BUILD_DIR)
	rm -f $(EXE)

all: clean dir $(EXE)
	@echo Complete

$(BUILD_DIR)/util.o:
	$(CXX) $(CXXFLAGS) -c -o $@ ../src/util.cpp

$(BUILD_DIR)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(EXE): $(SRC_OBJS)
	$(CXX) -o $@ $^ $(LIBS)
