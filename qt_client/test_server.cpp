#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h> // For nonBlock

#include <chrono>
#include <cstring>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <nlohmann/json.hpp>
#include <thread>

#include "message_data.hpp"
#include "socket.hpp"

using json = nlohmann::json;
using ChatHistory = std::list<Message>;
using std::cout;

constexpr int BACKLOG = 10;
constexpr int MAX_EVENTS = 10;

class Server {
  int fd;

 public:
  Server() {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      perror("Failed error create socket");
      exit(EXIT_FAILURE);
    }
  }
  ~Server() { close(fd); }

  std::vector<uint8_t> read_from_fd(const int fd, ssize_t length) {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> buffer;
    buffer.resize(2);
    ssize_t recived_bytes{};

    while (length > 0) {
      recived_bytes = recv(fd, buffer.data(), buffer.size(), 0);
      if (recived_bytes <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || recived_bytes == 0) {
          break;
        }
        bytes.clear();
        break;
      }
      length -= recived_bytes;
      bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + recived_bytes);
    }
    return bytes;
  }
  
  int to_listen(uint16_t port) {
    struct sockaddr_in srv_addr = {};
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    int opt = 1;
    int status = 0;

    if (!status && (status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) < 0) {
      perror("setcokopt(SO_REUSEADDR) failed");
    }
    if (!status && (status = bind(fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr))) < 0) {
      perror("Binding failed.");
    }
    if (!status && (status = listen(fd, BACKLOG)) < 0) {
      perror("Listening failed.");
    } else {
      std::cout << "Server ready. Listening on port " << port << std::endl;
    }
    return status;
  }
  int get_fd(void) const { return fd; }

};

bool turn_on_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags == -1) {
    return false;
  }
  
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    return false;
  }

  return true;
}

int main(void) {
  Server server;
  if ((server.to_listen(9090)) < 0) {
    exit(EXIT_FAILURE);
  }
  int serv_fd = server.get_fd();

  while (true) { 
  int client_fd = accept(server.get_fd(), nullptr, nullptr);
  if (client_fd > -1)
  std::cout << "Connection accept" << std::endl ;
  if (!turn_on_nonblock(client_fd)) {
    close(client_fd);
    fprintf (stderr, "failed turn_nonblock");
    exit(EXIT_FAILURE);
  }
  while (true) {
    byte data = SocketService::recv_all(client_fd);
    std::cout << "data: ";
    for (auto b : data) {
      std::cout << b ;
    }
    std::cout << std::endl;

    Request response;
    response.demarshaling(std::move(data));

    std::cout << "request(type = " << static_cast<uint8_t> (response.type) << "):" << response.body << std::endl; //TEST
    
  }
}
  return 0;
}