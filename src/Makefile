CXX ?= g++
EXE ?= test.exe
BUILD ?= build

IMGUI_DIR ?= ../lib/imgui-1.85
IMGUI_SOURCES =	$(IMGUI_DIR)/imgui.cpp        \
				$(IMGUI_DIR)/imgui_demo.cpp   \
				$(IMGUI_DIR)/imgui_draw.cpp   \
				$(IMGUI_DIR)/imgui_tables.cpp \
				$(IMGUI_DIR)/imgui_widgets.cpp

IMGUI_SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
				 $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

INCLUDES = ../include
SOURCES  =	socket_handler.cpp \
			user_list.cpp      \
			login.cpp          \
			gui.cpp            \
			quickshare.cpp

SRC_OBJS = $(addprefix $(BUILD)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
IMGUI_OBJS = $(addprefix $(BUILD)/, $(addsuffix .o, $(basename $(notdir $(IMGUI_SOURCES)))))
ALL_OBJS = $(IMGUI_OBJS) $(SRC_OBJS)

DEPS = $(addprefix build\, $(SOURCES:.cpp=.d))
SRC_INC_DIR = -I$(INCLUDES) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -O2 -Wall -std=c++17 -pedantic-errors
IMGUI_LIBS = -lglfw3 -lgdi32 -lopengl32 -limm32 -lws2_32

.PHONY: all clean clean_app

all: directory compile
ifneq ($(OS), Windows_NT)
	$(error Makefile only compatible for Windows system)
endif
	@echo $(DEPS)
	@echo Build Complete!

compile: $(EXE)
directory:
	if not exist build ( mkdir build )

# Build App Source
$(BUILD)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(SRC_INC_DIR) -MMD -MP -c -o $@ $<

# Build ImGui Source
$(BUILD)/%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(IMGUI_LIBS) -c -o $@ $<

# Build ImGui Backend Source
$(BUILD)/%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) $(IMGUI_LIBS) -I$(IMGUI_DIR) -c -o $@ $<

# Link Together
$(EXE): $(ALL_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(IMGUI_LIBS)

clean:
	del $(addprefix .\build\, $(SOURCES:.cpp=.o))

-include $(DEPS)