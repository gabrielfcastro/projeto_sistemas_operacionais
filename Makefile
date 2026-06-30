CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -pthread -I src

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

TARGET = dispatcher

.PHONY: all dispatcher test_integracao clean exemplo

all: dispatcher

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

dispatcher: $(BUILD_DIR)/dispatcher

$(BUILD_DIR)/dispatcher: $(SRC_DIR)/main.cpp $(SRC_DIR)/despachante.hpp \
                          $(SRC_DIR)/processo.hpp $(SRC_DIR)/filas.hpp \
                          $(SRC_DIR)/memoria.hpp $(SRC_DIR)/resource.hpp \
                          $(SRC_DIR)/filesystem.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/dispatcher

test_integracao: $(BUILD_DIR)/test_integracao
	@echo "\n>>> Rodando testes de integração"
	@./$(BUILD_DIR)/test_integracao

$(BUILD_DIR)/test_integracao: $(TEST_DIR)/test_integracao.cpp $(SRC_DIR)/despachante.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_DIR)/test_integracao.cpp -o $(BUILD_DIR)/test_integracao

exemplo: dispatcher
	@mkdir -p exemplo
	@printf '2, 0, 3, 4, 0, 0, 0, 0\n8, 0, 2, 8, 0, 0, 0, 0\n' > exemplo/processes.txt
	@printf '10\n3\nX, 0, 2\nY, 3, 1\nZ, 5, 3\n0, 0, A, 5\n0, 1, X\n2, 0, B, 2\n0, 0, D, 3\n1, 1, E\n' > exemplo/files.txt
	@printf '1,2,3,4,1,2,5,1,2,3,4,5\n7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1\n' > exemplo/string.txt
	./$(BUILD_DIR)/dispatcher exemplo/processes.txt exemplo/files.txt exemplo/string.txt

run: dispatcher
	./$(BUILD_DIR)/dispatcher test_sincrono/processes.txt test_sincrono/files.txt test_sincrono/string.txt

clean:
	rm -rf $(BUILD_DIR) exemplo