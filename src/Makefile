# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -D__OS_LINUX__ -Iimgui -Iimgui/backends -ISDL2

# EXE = new_synth

# Libraries
LIBS = -ldl -lstk -lpthread -lSDL2 -lGLEW -lGL

# Source files
SOURCES = main.cpp voice.cpp imgui/*.cpp imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_opengl3.cpp

# Output executable
TARGET = new_synth

# Build target
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

# Clean target
clean:
	rm -f $(TARGET)

