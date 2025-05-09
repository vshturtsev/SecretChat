#include "LoginService.hpp"
#include "../common/message_data.hpp"

#include <nlohmann/json.hpp>
#include <iostream>

LoginService::LoginService (int _fd, QObject *parent) : QObject(parent), fd(_fd) {

}

LoginService::~LoginService() {}

int LoginService::authentication (std::string& username, std::string& user_password, MsgType type) {
  std::string login = username;
  std::string password = user_password;

  AuthMessage auth_message(login, user_password);
  std::string json_message = MessageService::to_string(auth_message);
  
  MsgHeader headers = {
      .type = type,
      .len = static_cast<uint32_t>(json_message.size())
  };
  std::cout << "message: " << json_message.size() << "\n";
  std::vector<uint8_t> bytes_headers = marshaling(headers);
  // return 0; //Test
  ssize_t sent = send(fd, bytes_headers.data(), bytes_headers.size(), 0);
  if (sent < 0) {
      perror("failed send headers to server");
      return -1;
  }
  
  int status = send_message(json_message);
  return status;
}

int LoginService::send_message(std::string& message) {
  // std::cout << message << std::endl;
  // return 0;
  ssize_t sent = send(fd, message.data(), message.size(), MSG_NOSIGNAL);
  std::cout << "From client size message: " << sent << std::endl;
  if (sent < 0) {
      if (errno == EPIPE) {
          std::cout << "server disconnect: send" << '\n';
          return 0;
      }
      perror("faild send to server");
      return -1;
  }
  return 1;
}