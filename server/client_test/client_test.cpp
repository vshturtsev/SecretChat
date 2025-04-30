#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <system_error>

#include "../include/nlohmann_json.hpp"
using json = nlohmann::json;

// #define HOST "217.171.146.254"
#define HOST "127.0.0.1"

#define PORT 9095  // get by cl
#define USER_ID 123456  // get by cl

class MessageData {
  public:
  std::string content;
  int sender_id;
    
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(MessageData, content, sender_id);
};

int main(int argc, char *argv[]) {
  int sock = 0;
  struct sockaddr_in srv_addr = {};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket failed.");
    exit(EXIT_FAILURE);
  }

  srv_addr.sin_family = AF_INET;
  srv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, HOST, &srv_addr.sin_addr) <= 0) {
    throw std::system_error(errno, std::system_category(), "invalid adress");
  }

  if (connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
    throw std::system_error(errno, std::system_category(), "cannot connect");
  }

  int test = 5;
  while (--test) {
    std::string user_input;
    // std::cout << "Input message to send: (or '-exit' for finish):";
    std::getline(std::cin, user_input);

    if (user_input == "-exit") break;

    if (user_input.size() == 0) {
      std::cout << "You need to input text" << std::endl;
      continue;
    }

    MessageData msg = {user_input, USER_ID};
    std::string json_str = json(msg).dump();

    ssize_t bytes_sent = send(sock, json_str.c_str(), json_str.size(), 0);
    if (bytes_sent < 0) {
      std::cerr << "Send error: " << strerror(errno) << std::endl;
      break;
    }
    // std::cout << "Bytes sent: " << bytes_sent << std::endl;

    char response[1024] = {0};
    ssize_t bytes_received = recv(sock, response, sizeof(response), 0);
    if (bytes_sent <= 0) {
      std::cerr << "No response from server" << strerror(errno) << std::endl;
      break;
    }
    std::cout << "Response from server:\n" << response << std::endl;
  }

  close(sock);
}
