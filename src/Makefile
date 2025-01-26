# Default target platform
PLATFORM ?= linux

ifeq ($(PLATFORM), linux)
# Compiler and flags
	CXX = g++
	CXXFLAGS = -std=c++14 -pthread -Wall -D__OS_LINUX__ -Iimgui -Iimgui/backends -ISDL2 -fsanitize=address
# Libraries
	LIBS = -ldl -lstk -lpthread -lSDL2 -lGLEW -lGL
# Output executable
	TARGET = new_synth
else ifeq ($(PLATFORM), windows)
	CXX = x86_64-w64-mingw32-g++
    CXXFLAGS = -std=c++14 -D__OS_WINDOWS__ -Iimgui -Iimgui/backends -ISDL2 -Istk -Iimgui-knobs
    LIBS = -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglew32 -lwinmm -pthread
	LIBS += -Lstk/include -lstk

    TARGET = new_synth.exe
else
    $(error Unsupported platform: $(PLATFORM))
endif

# Source files
SOURCES = main.cpp voice.cpp imgui/*.cpp imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_opengl3.cpp imgui-knobs/imgui-knobs.cpp oscillator.cpp voiceAllocator.cpp


# Build target
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

# Clean target
clean:
	rm -f $(TARGET)

