# TARGET ?= quickshare
# SRC_DIRS ?= ./src
# CC = g++

# ifeq ($(OS), Windows_NT)
# 	RM = del
# 	SRCS := $(shell dir .\src /s /b | find ".cpp")
# 	OBJS := $(addsuffix .o,$(basename $(SRCS)))
# 	DEPS := $(OBJS:.o=.d)
# else 
# 	RM = rm
# endif

# INC_FLAGS := $(addprefix -I,./include)

# CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -O1 -std=c++17 -pedantic-errors -Wextra
# LDLIBS = -lws2_32 -lglfw3

# $(TARGET): $(OBJS)
# 	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS)
# 	$(RM) $(OBJS) $(DEPS)

# .PHONY: clean
# clean:
# 	$(RM) $(OBJS) $(DEPS)

# -include $(DEPS)

CXX = g++
#CXX = clang++

EXE = test.exe
IMGUI_DIR = ../lib/imgui-1.85
INCLUDES = ../include
SOURCES = socket_handler.cpp user_list.cpp login.cpp gui.cpp quickshare.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
# SOURCES += $(shell dir . /s /b | find ".cpp")
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
LINUX_GL_LIBS = -lGL

CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(INCLUDES)
CXXFLAGS += -g -Wall -Wformat -std=c++17
LIBS = 

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32 -lws2_32
	CFLAGS = $(CXXFLAGS)
endif

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	del $(EXE) $(OBJS)
