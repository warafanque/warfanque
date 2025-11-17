# Makefile for Sphere Gouraud Shading Application

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -O2 -Wall -mwindows

# Linker flags
LDFLAGS = -lgdi32 -lcomctl32 -luser32 -lkernel32

# Target executable
TARGET = sphere_gouraud.exe

# Source files
SOURCES = sphere_gouraud.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
