CXX := g++
CXXFLAGS := -Wall -std=c++17
SRC   := src/main.cpp
TARGET := main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET)