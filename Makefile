CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -Wextra
INCLUDES = -I./
LIBS = -lspdlog -lfmt
SRC = main.cpp Book.cpp Order.cpp Security.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = order_matching_engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
