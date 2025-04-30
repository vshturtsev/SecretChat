#include <chrono>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
// #include "include/nlohmann_json.hpp"
#include <nlohmann/json.hpp>

class MessageData;

using json = nlohmann::json;
using Timepoint = std::chrono::system_clock::time_point;
using Message = std::pair<Timepoint, MessageData>;
using ChatHistory = std::list<Message>;

std::string timePointToString(const std::chrono::system_clock::time_point &tp) {
  // Конвертируем в time_t (секунды с эпохи Unix)
  std::time_t time = std::chrono::system_clock::to_time_t(tp);

  // Преобразуем в структуру tm (UTC или локальное время)
  // std::tm tm = *std::gmtime(&time);  // Для UTC используйте gmtime
  std::tm tm = *std::localtime(&time); // Для локального времени

  // Форматируем в строку (например, "2024-05-10 14:30:00")
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

// // Хешер для time_point (преобразует в наносекунды)
// struct TimepointHash {
//   size_t operator()(const Timepoint& tp) const {
//       return std::chrono::duration_cast<std::chrono::nanoseconds>(
//           tp.time_since_epoch()
//       ).count();
//   }
// };

class MessageData {
public:
  std::string content;
  int sender_id;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(MessageData, content, sender_id);
};

void print_chat(ChatHistory chat) {
  std::cout << "Chat History:\n";
  for (auto it = chat.begin(); it != chat.end(); ++it) {
    std::cout << timePointToString(it->first) << " (user "
              << it->second.sender_id << "):\n";
    std::cout << it->second.content << '\n' << std::endl;
  }
}

void print_message(Message msg) {
  std::cout << timePointToString(msg.first) << " (user " << msg.second.sender_id
            << "):\n";
  std::cout << msg.second.content << '\n' << std::endl;
}

void proccess_message(const char *buffer, ChatHistory &chat) {
  auto received_json = json::parse(buffer);
  MessageData msg_data = received_json.get<MessageData>();
  Timepoint now = std::chrono::system_clock::now();
  Message msg{now, msg_data};
  chat.push_back(msg);
  print_chat(chat);
}

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

  int get_fd(void) const { return fd; }

  void start_listen(int port) {
    struct sockaddr_in srv_addr = {};
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      perror("setcokopt(SO_REUSEADDR) failed");
    }

    if (bind(fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
      perror("Binding failed.");
      exit(EXIT_FAILURE);
    }

    if (listen(fd, BACKLOG) == 0) {
      std::cout << "Server ready. Listening on port " << port << std::endl;
    } else {
      perror("Listening failed.");
      exit(EXIT_FAILURE);
    }
  }
};

class ClientSession {
public:
  std::string name;
  bool auth_status = false;
};

void broadcast_message(int autor_fd,
                       std::unordered_map<int, ClientSession> clients_pool,
                       std::string message) {
  for (const auto &[fd, client_session] : clients_pool) {
    if (fd != autor_fd && client_session.auth_status) {
      if (send(fd, message.data(), message.size(), 0) < 0) {
        perror("Error send message.");
      }
    }
  }
}

int main(void) {
  // if (fopen("config.txt"))
  // else {
  //     take_args(); // scanf();
  // }
  Server server;
  server.start_listen(9090);
  int serv_fd = server.get_fd();
  ChatHistory chat;

  int epoll_fd;
  struct epoll_event event, ev_list[MAX_EVENTS];

  epoll_fd = epoll_create1(0);
  event.events = EPOLLIN;
  event.data.fd = serv_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serv_fd, &event);

  std::unordered_map<int, ClientSession> clients_pool;

  while (true) {
    int ready_fd = epoll_wait(epoll_fd, ev_list, MAX_EVENTS, -1);
    for (int i = 0; i < ready_fd; i++) {
      if (ev_list[i].data.fd == serv_fd) {
        int new_sock = accept(server.get_fd(), nullptr, nullptr);
        clients_pool.emplace(new_sock, ClientSession{"name", false});
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = new_sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);

      } else {
        std::string buffer;
        buffer.resize(2048);
        int client_fd = ev_list[i].data.fd;
        auto it = clients_pool.find(client_fd);
        if (it != clients_pool.end()) {
          if (it->second.auth_status) {
            int bytes = recv(client_fd, buffer.data(), buffer.size(), 0);
            if (bytes <= 0) {
              epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &event);
              clients_pool.erase(client_fd);
            } else {
              std::cout << "Received request(" << bytes << " bytes): \n"
                        << buffer << std::endl;
              broadcast_message(client_fd, clients_pool, buffer);
              // proccess_message(buffer, chat);
            }
          } else {
            // not auth
          }
          // clients.erase(ev_list[i].data.fd);
        }
      }
    }
  }
  return 0;
}