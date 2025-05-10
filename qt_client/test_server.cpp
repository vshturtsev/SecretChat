#include <fcntl.h> // For nonBlock
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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

    if (!status && (status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                                        sizeof(opt))) < 0) {
      perror("setcokopt(SO_REUSEADDR) failed");
    }
    if (!status && (status = bind(fd, (struct sockaddr *)&srv_addr,
                                  sizeof(srv_addr))) < 0) {
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

  int accept(sockaddr *addr, socklen_t *addr_len) {
    int connect_fd = ::accept(fd, addr, addr_len);
    if (connect_fd < 0) {
      std::cerr << "failed connection socket accept\n";
    }
    if (connect_fd > 0 && SocketService::turn_on_nonblock(connect_fd) < 0) {
      std::cerr << "failed nonblock socket\n";
      close(connect_fd);
      connect_fd = -1;
    }
    return connect_fd;
  }
};

int main(void) {
  Server server;
  if ((server.to_listen(9090)) < 0) {
    exit(EXIT_FAILURE);
  }
  int serv_fd = server.get_fd();
  int epoll_fd = epoll_create1(0);
  struct epoll_event event, ev_list[MAX_EVENTS];

  event.events = EPOLLIN;
  event.data.fd = serv_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serv_fd, &event);

  while (true) {
    int ready_fd = epoll_wait(epoll_fd, ev_list, MAX_EVENTS, -1);
    if (ready_fd < 0) {
      if (errno == EINTR)
        continue;

      perror("failed epoll_wait");
      break;
    }
    cout << "client ready: " << ready_fd << "\n";

    for (int i = 0; i < ready_fd; i++) {
      if (ev_list[i].data.fd == serv_fd) {
        int new_sock = server.accept(nullptr, nullptr);
        if (new_sock < 0) {
          continue;
        }
        cout << "connected: " << new_sock << "\n";
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = new_sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);

      } else if (ev_list[i].events & EPOLLIN) { // событие поступления данных
        int fd = ev_list[i].data.fd;
        cout << "client session: " << fd << "\n";
        byte data = SocketService::recv_all(fd);
        // std::cout << "data: ";
        // for (auto b : data) {
        //   std::cout << b;
        // }
        // std::cout << std::endl;

        Request response;
        response.demarshaling(std::move(data));

        std::cout << "request(type = " << static_cast<int>(response.type)
                  << "):" << response.body << std::endl; // TEST

      } else if (ev_list[i].events &
                 (EPOLLRDHUP | EPOLLERR |
                  EPOLLHUP)) { // закрытие удаленного сокета либо ошибка
        std::cerr << "failed events\n";
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev_list[i].data.fd, &event);
        close(ev_list[i].data.fd);
      }
    }
  }
  return 0;
}