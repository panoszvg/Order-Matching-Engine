CXX = g++
CXXFLAGS = -std=c++17 -pthread
INCLUDES = -I./

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    LIBS = -lspdlog -lfmt -lboost_system
    LIBS_TEST = $(LIBS) -lgmock_main -lgmock -lgtest
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    LIBS = -lspdlog -lfmt -lws2_32 -lmswsock
    LIBS_TEST = $(LIBS) -lgmock_main -lgmock -lgtest
endif

SRC_CODE = 	$(wildcard *.cpp) \
			$(wildcard tcp/*.cpp) \
			$(wildcard strategy/*.cpp) \
			$(wildcard messages/FixMessage/*.cpp) \
			$(wildcard messages/SimpleMessage/*.cpp) \
			$(wildcard benchmark/*.cpp)
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
