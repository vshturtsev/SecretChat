CC = g++
CFLAGS = -std=c++17
DATAFLAGS = -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi -L/usr/local/lib
DATALIB = -lmongocxx -lbsoncxx

BIN_DIR = bin
OBJ_DIR = obj

all: main

client: ../client/src/main.cpp
	$(CC) $^ -o $(BIN_DIR)/client

main: common.o main.cpp 
# $(CC) $^ -o $(BIN_DIR)/server
	$(CC) $^ $(DATAFLAGS) -o server $(DATALIB)

common.o: ../common/message_data.cpp ../common/message_data.hpp
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: runserver clean killserver clang

test_client:
	$(CC) client_test/client_test.cpp -o client

clang:
	clang-format-18 -i *.cpp *.hpp

killserver:
	killall server

test_main:
	g++ -std=c++20 -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi -L/usr/local/lib  test_main.cpp -lmongocxx -lbsoncxx




runserver:
	@echo "Start server"
	@nohup $(BIN_DIR)/server > server.log 2>&1 &

clean:
	rm -r $(OBJ_DIR)/*.o