## ----------------------------------------------------------------------
## Quickshare Makefile for Unix ONLY
## ----------------------------------------------------------------------

CXX = g++
EXE = qs
BUILD_DIR = build
TYPE = DEBUG

IMGUI_DIR ?= ../lib/imgui-1.85
IMGUI_SOURCES =	$(IMGUI_DIR)/imgui.cpp        \
				$(IMGUI_DIR)/imgui_demo.cpp   \
				$(IMGUI_DIR)/imgui_draw.cpp   \
				$(IMGUI_DIR)/imgui_tables.cpp \
				$(IMGUI_DIR)/imgui_widgets.cpp

IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
				 $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

FILE_DIALOG_INCLUDE = "../lib/nativefiledialog/src/include"
FILE_DIALOG_LIB_PATH = ../lib/nativefiledialog/build/lib/Release/x64/

# Shared Libs
SHARED_DIR   = "../shared"
UTIL_OBJ     = "../shared/util/util.o"
MEM_POOL_OBJ = "../shared/mem_pool/mem.o"

LOCK_FREE_QUEUE_HDR = "../lib/LockFreeQueueCpp11.h"

INCLUDES = ../include
SOURCES  =	users.cpp            \
			file_manager.cpp     \
			transfer_manager.cpp \
			network.cpp          \
			context.cpp          \
			login_menu.cpp       \
			main_menu.cpp        \
			quickshare.cpp

UNAME_S    := $(shell uname -s)
SRC_OBJS   = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
IMGUI_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(IMGUI_SOURCES)))))
ALL_OBJS   = $(IMGUI_OBJS) $(SRC_OBJS) $(FILE_DIALOG_OBJ) $(UTIL_OBJ) $(MEM_POOL_OBJ)

DEPS = $(addprefix build/, $(SOURCES:.cpp=.d))
SRC_INC_DIR = -I$(INCLUDES) -I$(IMGUI_DIR) \
			  -I$(IMGUI_DIR)/backends -I$(FILE_DIALOG_INCLUDE) \
			  -I../shared/util -I../shared/mem_pool -I../shared
CXXFLAGS += -O2 -Wall -std=c++17 -pedantic-errors -Werror -Wextra

ifeq ($(UNAME_S), Linux)
	LIBS += `pkg-config --libs gtk+-3.0` -lGL \
			-lpthread
endif
ifeq ($(UNAME_S), Darwin)
	CXXFLAGS += -Wextra -Wno-overlength-strings \
				-Wno-deprecated-declarations \
				-Wno-unused-private-field
	LIBS += -framework OpenGL
endif

LIBS += `pkg-config --static --libs glfw3` \
		-L$(FILE_DIALOG_LIB_PATH) -lnfd \
		-lssl -lcrypto

ifeq ($(TYPE), RELEASE)
	LINKER += -static -s -lc -lstdc++
endif
ifeq ($(TYPE), DEBUG)
	CXXFLAGS += -DDEBUG_MODE
endif

.PHONY: all clean directory \
		install_dir build_icon install_nfd \
		install_imgui install_deps install_stbimage

all: directory compile
ifeq ($(OS), Windows_NT)
	$(error Makefile only compatible for Linux)
endif
	@echo Build Complete!

compile: $(EXE)

directory:
	mkdir -p build

install_deps: install_nfd \
			  install_imgui \
			  install_stbimage

install_stbimage:
	mkdir -p ../lib
	cd ../lib && wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

install_nfd:
	mkdir -p ../lib
	cd ../lib && git clone https://github.com/mlabbe/nativefiledialog
ifeq ($(UNAME_S), Linux)
	cd ../lib/nativefiledialog/build/gmake_linux && make
endif
ifeq ($(UNAME_S), Darwin)
	cd ../lib/nativefiledialog/build/gmake_macosx && make
endif
install_imgui:
	mkdir -p ../lib
	cd ../lib && git clone -b v1.85 https://github.com/ocornut/imgui/
	cd ../lib && mv imgui imgui-1.85
	
clean:
	rm -r $(BUILD_DIR)
	rm -f $(EXE)

# Build shared util
$(UTIL_OBJ):
	@$(MAKE) -C $(SHARED_DIR)/util/

# Build shared mem pool
$(MEM_POOL_OBJ):
	@$(MAKE) -C $(SHARED_DIR)/mem_pool/

$(BUILD_DIR)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(IMGUI_LIBS) -c -o $@ $<

# Build ImGui Backend Source
$(BUILD_DIR)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) $(IMGUI_LIBS) -I$(IMGUI_DIR) -c -o $@ $<

# Build App Source
$(BUILD_DIR)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(SRC_INC_DIR) -MMD -MP -c -o $@ $<

# Link Together
$(EXE): $(ALL_OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(CXXFLAGS) $(LINKER)

-include $(DEPS)