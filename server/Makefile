CC = g++
CFLAGS =

BIN_DIR = bin
OBJ_DIR = obj

all: main

main: main.cpp
	$(CC) $^ -o $(BIN_DIR)/server

$(OBJ_DIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: runserver clean killserver clang

test_client:
	$(CC) client_test/client_test.cpp -o client

clang:
	clang-format-18 -i *.cpp *.hpp

killserver:
	killall server

runserver:
	@echo "Start server"
	@nohup $(BIN_DIR)/server > server.log 2>&1 &

clean:
	rm -r $(OBJ_DIR)/*.o