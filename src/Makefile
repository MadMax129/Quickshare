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

IMGUI_DIR ?= ../lib/imgui-1.85
IMGUI_SOURCES =	$(IMGUI_DIR)/imgui.cpp        \
				$(IMGUI_DIR)/imgui_demo.cpp   \
				$(IMGUI_DIR)/imgui_draw.cpp   \
				$(IMGUI_DIR)/imgui_tables.cpp \
				$(IMGUI_DIR)/imgui_widgets.cpp

IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
				 $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

INCLUDES = ../include
SOURCES  =	file_menu.cpp	   \
			chat_menu.cpp      \
			socket_handler.cpp \
			user_list_menu.cpp      \
			login_menu.cpp          \
			gui.cpp            \
			quickshare.cpp

SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
IMGUI_OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(IMGUI_SOURCES)))))
ALL_OBJS = $(IMGUI_OBJS) $(SRC_OBJS)

DEPS = $(addprefix build\, $(SOURCES:.cpp=.d))
SRC_INC_DIR = -I$(INCLUDES) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -O2 -Wall -std=c++17 -pedantic-errors
IMGUI_LIBS = -lglfw3 -lgdi32 -lopengl32 -limm32 -lws2_32

ifeq ($(TYPE), RELEASE)
	LINKER += -static -s -static-libgcc -static-libstdc++ -mwindows
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

# Build ImGui Source
$(BUILD_DIR)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(IMGUI_LIBS) -c -o $@ $<

# Build ImGui Backend Source
$(BUILD_DIR)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) $(IMGUI_LIBS) -I$(IMGUI_DIR) -c -o $@ $<

# Link Together
$(EXE): $(ALL_OBJS)
	$(CXX) -o $@ $^ ./icon/icon.res $(CXXFLAGS) $(LINKER) $(IMGUI_LIBS)

-include $(DEPS)