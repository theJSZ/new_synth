# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -D__OS_LINUX__

# Libraries
LIBS = -lstk -lpthread -lSDL2

# Source files
SRCS = main.cpp voice.cpp

# Output executable
TARGET = new_synth

# Build target
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# Clean target
clean:
	rm -f $(TARGET)

