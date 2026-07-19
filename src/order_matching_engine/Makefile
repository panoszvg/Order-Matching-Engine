MAKEFLAGS += -j4

CXX = g++
CXXFLAGS = -std=c++17 -pthread
INCLUDES = -I./ -I./core -I./tests

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    LIBS = -lspdlog -lfmt -lboost_system
    LIBS_TEST = $(LIBS) -lgmock_main -lgmock -lgtest
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    LIBS = -lspdlog -lfmt -lws2_32 -lmswsock
    LIBS_TEST = $(LIBS) -lgmock_main -lgmock -lgtest
endif

SRC_CODE = 	$(filter-out core/main.cpp, $(wildcard core/*.cpp)) \
			$(wildcard tcp/*.cpp) \
			$(wildcard strategy/*.cpp) \
			$(wildcard messages/FixMessage/*.cpp) \
			$(wildcard messages/SimpleMessage/*.cpp) \
			$(wildcard benchmark/*.cpp)
SRC_MAIN = core/main.cpp
SRC_TEST = $(wildcard tests/core/*.cpp) \
           $(wildcard tests/strategy/*.cpp) \
           $(wildcard tests/messages/*.cpp) \
           $(wildcard tests/tcp/*.cpp)

BIN_DIR = bin

OBJ_CODE = $(addprefix $(BIN_DIR)/,$(SRC_CODE:.cpp=.o))
OBJ_MAIN = $(addprefix $(BIN_DIR)/,$(SRC_MAIN:.cpp=.o)) $(OBJ_CODE)
OBJ_TEST = $(addprefix $(BIN_DIR)/,$(SRC_TEST:.cpp=.o)) $(OBJ_CODE)

TARGET_MAIN = order_matching_engine
TARGET_TEST = tests/run_tests

.PHONY: all test clean

all: $(TARGET_MAIN)

$(TARGET_MAIN): $(OBJ_MAIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test: $(TARGET_TEST)
	./$(TARGET_TEST)

$(TARGET_TEST): $(OBJ_TEST)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS_TEST)

clean:
	rm -rf $(BIN_DIR)
	rm -f $(TARGET_MAIN) $(TARGET_TEST)
