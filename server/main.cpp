#include <fcntl.h> // For nonBlock
#include <sys/epoll.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "../common/message_data.hpp"
#include "../common/socket.hpp"
#include "../mongo/mongo.hpp"

// using Timepoint = std::chrono::system_clock::time_point;
// using Message = std::pair<Timepoint, MessageData>;
// using ChatHistory = std::list<Message>;
using std::cout;

class ClientSession {

public:
  ClientSession() = default;
  ClientSession(int _fd) : fd(_fd) {}
  const int fd = 0;
  std::string name;
  bool auth_status = false;
};

using ClientPoll = std::unordered_map<int, ClientSession>;

constexpr int BACKLOG = 10;
constexpr int MAX_EVENTS = 10;
void broadcast_message(int autor_fd, const ClientPoll clients_pool,
                       const std::string message) {
  // std::cout << "broadcast: " << message << std::endl;
  for (const auto &[fd, client_session] : clients_pool) {
    if (fd != autor_fd && client_session.auth_status) {
      if (send(fd, message.data(), message.size(), 0) < 0) {
        perror("Error send message.");
      }
    }
  }
}

bool registration(ClientSession &client, const std::string &message) {
  AuthMessage auth_data = MessageService::from_string<AuthMessage>(message);
  User user;
  if (user.add_user(auth_data.get_login(), auth_data.get_password())) {
    client.auth_status = true;
    client.name = auth_data.get_login();
  }

  return client.auth_status;
}

void authorization(ClientSession &client, std::string &message) {
  AuthMessage auth_data = MessageService::from_string<AuthMessage>(message);
  User user;

  if (user.compare_user(auth_data.get_login(), auth_data.get_password())) {
    client.auth_status = true;
    client.name = auth_data.get_login();
    std::cout << "Good Auth request (" << message.size() << " bytes): \n"
              << message << "From: " << client.name << std::endl;
  } else {
    std::cout << "Bad auth request (" << message.size() << " bytes): \n"
              << message << "From: " << client.name << std::endl;
  }
}

bool chat_(ClientSession &client, std::string &message,
           std::string &message_to_broadcast) {
  if (client.auth_status) {
    Message restore_msg = MessageService::from_string<Message>(message);
    ChatMessage chat_msg(std::move(restore_msg), client.name);
    chat_msg.update_time();
    message_to_broadcast = MessageService::to_string(std::move(chat_msg));
    return true;
  }
  return false;
}

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

  int handle_client(ClientSession &client_session, ClientPoll &clients_pool) {
    byte data = SocketService::recv_all(client_session.fd);
    std::cout << "data size: " << data.size() << '\n';
    if (data.size() <= 0) {
      return -1;
    }
    Request response;
    response.demarshaling(std::move(data));

    std::string message_to_broadcast;
    switch (response.type) {
    case ReqType::Reg:
      registration(client_session, response.body);
      break;
    case ReqType::Auth:
      authorization(client_session, response.body);
      break;

    case ReqType::Chat:
      std::cout << "message: " << response.body
                << " size: " << response.body.size() << "\n\n";
      if (chat_(client_session, response.body, message_to_broadcast)) {
        broadcast_message(client_session.fd, clients_pool,
                          message_to_broadcast);
      } else {
        cout << "not auth\n";
      }
      break;
    default:
      break;
    }
    return 0;
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
  // if (fopen("config.txt"))
  // else {
  //     take_args(); // scanf();
  // }

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

  ClientPoll clients_pool;

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
        clients_pool.emplace(new_sock, ClientSession{new_sock});
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = new_sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);

      } else if (ev_list[i].events & EPOLLIN) { // событие поступления данных
        int fd = ev_list[i].data.fd;
        cout << "client session: " << fd << "\n";
        auto client = clients_pool.find(fd);
        if (client != clients_pool.end()) {
          int status = server.handle_client(client->second, clients_pool);
          if (status == -1) {
            cout << "disconnect client\n";
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->first, &event);
            clients_pool.erase(client->first);
            close(fd);
          } else if (status > 0) {
            perror("failed handle client");
            close(fd);
          }
        }
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