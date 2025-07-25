CXX = g++
CXXFLAGS = -std=c++17 -pthread
INCLUDES = -I./

LIBS = -lspdlog -lfmt -lws2_32 -lmswsock
LIBS_TEST = $(LIBS) -lgmock_main -lgmock -lgtest

SRC_CODE = 	Logger.cpp Order.cpp Security.cpp Book.cpp Parser.cpp \
			SecurityProvider.cpp strategy/PriceTimePriorityStrategy.cpp \
			messages/FixMessage/FixMessage.cpp messages/FixMessage/FixMap.cpp \
			messages/FixMessage/FixField.cpp messages/SimpleMessage/SimpleMessage.cpp \
			benchmark/Benchmark.cpp \
			tcp/TcpServer.cpp tcp/JsonOrderHandler.cpp tcp/TcpSession.cpp
SRC_MAIN = main.cpp
SRC_TEST = $(wildcard test/*.cpp)

OBJ_CODE = $(SRC_CODE:.cpp=.o)
OBJ_MAIN = $(SRC_MAIN:.cpp=.o) $(SRC_CODE:.cpp=.o)
OBJ_TEST = $(SRC_TEST:.cpp=.o) $(SRC_CODE:.cpp=.o)

TARGET_MAIN = order_matching_engine
TARGET_TEST = test/run_tests

.PHONY: all test clean

all: $(TARGET_MAIN)

$(TARGET_MAIN): $(OBJ_MAIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test: $(TARGET_TEST)

$(TARGET_TEST): $(OBJ_TEST)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS_TEST)

clean:
	rm -f $(OBJ_MAIN) $(TARGET_MAIN) $(OBJ_TEST) $(TARGET_TEST)
