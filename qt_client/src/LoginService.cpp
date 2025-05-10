#include "LoginService.hpp"
#include "message_data.hpp"
#include "socket.hpp"

#include <nlohmann/json.hpp>
#include <iostream>

LoginService::LoginService (int _fd, QObject *parent) : QObject(parent), fd(_fd) {

}

LoginService::~LoginService() {}

int LoginService::authentication (std::string& username, std::string& user_password, ReqType type) {
  AuthMessage auth_message(username, user_password);
  std::string json_data = MessageService::to_string(std::move(auth_message));
  Request request(type, std::move(json_data));
  std::cout << "request(type = " << static_cast<int>(request.type) << "):" << json_data << std::endl; //TEST

  bytes data = request.marshaling();  
  int status = SocketService::send_all(fd, data);
  std::cout << "sended: " << status << std::endl;
  return status;
}

int LoginService::send_message(std::string& message) {
  std::cout << "send_message: " << message << std::endl;
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