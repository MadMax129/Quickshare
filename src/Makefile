## ----------------------------------------------------------------------
## Quickshare Makefile for Mac/Linux
## ----------------------------------------------------------------------

CXX = g++
EXE = qs
BUILD_DIR = build
TYPE = DEBUG

# Install Directories
# INSTALL_DIR = "/home/"$(USER)"/Desktop/"
# ICON_PATH   = "C:\Program Files\Quickshare\icon\logo.png"

INCLUDES = ../include
SOURCES  =	main_menu.cpp	 \
			gui_style.cpp    \
			gui.cpp          \
			database.cpp     \
			network.cpp	     \
			file_manager.cpp \
			quickshare.cpp

SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
IMGUI_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(IMGUI_SOURCES)))))
ALL_OBJS = $(IMGUI_OBJS) $(SRC_OBJS)

DEPS = $(addprefix build/, $(SOURCES:.cpp=.d))
SRC_INC_DIR = -I$(INCLUDES) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -O2 -Wall -std=c++17 -pedantic-errors -lm -lkernel32 -lole32 -luuid
IMGUI_LIBS = -lglfw3 -lopengl32 -lws2_32 -lpthread


ifeq ($(TYPE), RELEASE)
	LINKER += -static -s -static-libgcc -static-libstdc++
# Add CXXFALHS RELEASE 
endif

ifeq ($(TYPE), DEBUG)
	CXXFLAGS += -DDEBUG_MODE
endif

.PHONY: all clean directory

all: directory compile
	@echo Build Complete!

compile: $(EXE)

directory:
	mkdir -p $(BUILD_DIR)

# install_all:
# 	if not exist $(INSTALL_DIR) ( mkdir $(INSTALL_DIR) )
# 	if not exist $(INSTALL_DIR)\icon ( mkdir $(INSTALL_DIR)\icon )
# 	copy ..\images\logo.png $(INSTALL_DIR)\icon
	
# Eventually move exe into its program files and make shortcut

clean:
	rm -r $(BUILD_DIR)

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