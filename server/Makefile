CC = g++
CFLAGS = -std=c++17
DATAFLAGS = -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi 
DATALIB = -L/usr/local/lib -lmongocxx -lbsoncxx

BIN_DIR = bin
OBJ_DIR = obj


OBJS_DIR_CLIENT = $(OBJ_DIR)/client.o $(OBJ_DIR)/common.o $(OBJ_DIR)/socket.o
OBJS_DIR_SERVER = $(OBJ_DIR)/server.o $(OBJ_DIR)/mongo.o $(OBJ_DIR)/common.o $(OBJ_DIR)/socket.o
OBJS_CLIENT = client.o common.o socket.o
OBJS_SERVER = server.o mongo.o socket.o common.o

all: main

client: obj_dir $(OBJS_CLIENT)
	$(CC) $(OBJS_DIR_CLIENT) -o client 

server: obj_dir $(OBJS_SERVER)
# $(CC) $^ -o $(BIN_DIR)/server
	$(CC) $(OBJS_DIR_SERVER) -o server $(DATALIB) 

obj_dir:
	@mkdir -p $(OBJ_DIR)

server.o: ../server/main.cpp
	$(CC) -c $< -o $(OBJ_DIR)/$@ $(DATAFLAGS) $(CFLAGS)

client.o: ../client/src/main.cpp
	$(CC) -c $< -o $(OBJ_DIR)/$@ $(CFLAGS)

common.o: ../common/message_data.cpp ../common/message_data.hpp
	$(CC) -c $< -o $(OBJ_DIR)/$@ $(CFLAGS)

mongo.o: ../mongo/mongo.cpp ../mongo/mongo.hpp
	$(CC) -c $< -o $(OBJ_DIR)/$@  $(DATAFLAGS) $(CFLAGS)

socket.o: ../common/socket.cpp ../common/socket.hpp
	$(CC) -c $< -o $(OBJ_DIR)/$@ $(CFLAGS)

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
	rm -rf $(OBJ_DIR)/*.o