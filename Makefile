CXX := g++
CXXFLAGS := -Wall -std=c++17 -pthread
SRC   := src/reader.cpp src/message.cpp src/peer.cpp src/requests.cpp src/parrallel.cpp src/torrent_engine.cpp
TARGET := main.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) -lcurl -lssl -lcrypto

clean:
	rm -rf $(TARGET)