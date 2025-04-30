#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

constexpr int DEFAULT_PORT = 9090;
constexpr int MAX_EVENTS = 10;
constexpr int BACKLOG = 10;
constexpr int MSG_BUFFER = 4096;

class Server {
  int port = DEFAULT_PORT;
  int fd;

  int socket(int port = DEFAULT_PORT) {
    if ((fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket() failed.");
      return 1;
    }
    return 0;
  }

  int bind() {}

public:
  bool start(int port = DEFAULT_PORT) {
    if (!socket(port))
      return false;
    if (!bind())
      return false;
    if (!listen())
      return false;
  }
};

int main(int argc, char *argv[]) {
  int srv_sock;
  int new_sock;
  int epoll_fd = epoll_create1(0);
  struct epoll_event event, events[MAX_EVENTS];

  if ((srv_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creating failed.");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in srv_addr = {};
  // memset(&srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_addr.s_addr = INADDR_ANY;
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_port = htons(PORT);

  int opt = 1;
  if (setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setcokopt(SO_REUSEADDR) failed");
  }

  if (bind(srv_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
    perror("Binding failed.");
    exit(EXIT_FAILURE);
  }

  if (listen(srv_sock, BACKLOG) == 0) {
    std::cout << "Server ready. Listening on port " << PORT << std::endl;
  } else {
    perror("Listening failed.");
  }

  event.events = EPOLLIN;
  event.data.fd = srv_sock;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_sock, &event);

  while (1) {
    int n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < n_ready; ++i) {
      if (events[i].data.fd == srv_sock) {
        int new_client = accept(srv_sock, nullptr, nullptr);
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = new_client;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client, &event);
      } else {
        char buffer[1024] = {0};
        int bytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
          close(events[i].data.fd);
        } else {
          std::cout << "Received request(" << bytes << " bytes): \n"
                    << buffer << std::endl;

          const char *response = "Message received\n";
          if (send(events[i].data.fd, response, strlen(response), 0) < 0) {
            perror("Send failed");
            break;
          }
        }
      }
    }
  }
  close(epoll_fd);
  close(srv_sock);

  return 0;
}