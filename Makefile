CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -pthread -I src

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

TESTS_BIN = $(BUILD_DIR)/test_despachante \
            $(BUILD_DIR)/test_filas \
            $(BUILD_DIR)/test_files \
            $(BUILD_DIR)/test_integracao \
            $(BUILD_DIR)/test_memoria \
            $(BUILD_DIR)/test_processo \
            $(BUILD_DIR)/test_resource

.PHONY: all dispatcher test test_integracao clean exemplo run

all: dispatcher

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

dispatcher: $(BUILD_DIR)/dispatcher

$(BUILD_DIR)/dispatcher: $(SRC_DIR)/main.cpp $(SRC_DIR)/despachante.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/dispatcher

$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

test: $(TESTS_BIN)
	@./run_tests.sh

t_%: $(BUILD_DIR)/test_%
	./$(BUILD_DIR)/test_$*

test_integracao: $(BUILD_DIR)/test_integracao
	@./$(BUILD_DIR)/test_integracao

clean:
	rm -rf $(BUILD_DIR) exemplo