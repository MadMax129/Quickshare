## ----------------------------------------------------------------------
## Quickshare Makefile
## ----------------------------------------------------------------------

CXX = g++
EXE = qs.exe
BUILD_DIR = build
TYPE = DEBUG

# Install Directories
INSTALL_DIR = "C:\Program Files\Quickshare"
ICON_PATH   = "C:\Program Files\Quickshare\icon\logo.png"


INCLUDES = ../include
SOURCES  =	network.cpp	   \
			quickshare.cpp

SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
IMGUI_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(IMGUI_SOURCES)))))
ALL_OBJS = $(IMGUI_OBJS) $(SRC_OBJS)

DEPS = $(addprefix build\, $(SOURCES:.cpp=.d))
SRC_INC_DIR = -I$(INCLUDES)
CXXFLAGS += -O2 -Wall -std=c++17 -pedantic-errors
LIBS = -limm32 -lws2_32

ifeq ($(TYPE), RELEASE)
	LINKER += -static -s -static-libgcc -static-libstdc++ -mwindows
endif

ifeq ($(TYPE), DEBUG)
	CXXFLAGS += -DDEBUG_MODE
endif

.PHONY: all clean directory install_all build_icon

all: directory compile
ifneq ($(OS), Windows_NT)
	$(error Makefile only compatible for Windows system)
endif
	@echo Build Complete!

compile: $(EXE)

directory:
	@if not exist $(BUILD_DIR) ( mkdir $(BUILD_DIR) )

install_all:
	if not exist $(INSTALL_DIR) ( mkdir $(INSTALL_DIR) )
	if not exist $(INSTALL_DIR)\icon ( mkdir $(INSTALL_DIR)\icon )
	copy ..\images\logo.png $(INSTALL_DIR)\icon
	
# Eventually move exe into its program files and make shortcut

clean:
	del $(addprefix .\build\, $(SOURCES:.cpp=.o))
	rmdir /s /q $(INSTALL_DIR)

build_icon:
	windres .\icon\resource.rc -O coff -o .\icon\icon.res

# TO create shortcut mklink qs_short.lnk qs.exe

# Build App Source
$(BUILD_DIR)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(SRC_INC_DIR) -MMD -MP -c -o $@ $<

# Link Together
$(EXE): $(ALL_OBJS)
	$(CXX) -o $@ $^ ./icon/icon.res $(CXXFLAGS) $(LINKER) $(LIBS)

-include $(DEPS)