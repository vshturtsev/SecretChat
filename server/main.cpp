#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <chrono>
#include <unistd.h>

#include <cstring>
#include <string>
#include <iostream>
#include <list>
#include <utility>

#include "include/nlohmann_json.hpp"

class MessageData;

using json = nlohmann::json;
using Timepoint = std::chrono::system_clock::time_point;
using Message = std::pair<Timepoint, MessageData>;
using ChatHistory = std::list<Message>;

std::string timePointToString(const std::chrono::system_clock::time_point& tp) {
  // Конвертируем в time_t (секунды с эпохи Unix)
  std::time_t time = std::chrono::system_clock::to_time_t(tp);
  
  // Преобразуем в структуру tm (UTC или локальное время)
  // std::tm tm = *std::gmtime(&time);  // Для UTC используйте gmtime
  std::tm tm = *std::localtime(&time);  // Для локального времени

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
  for(auto it = chat.begin(); it != chat.end(); ++it) {
    std::cout << timePointToString(it->first) << " (user " << it->second.sender_id << "):\n";
    std::cout << it->second.content << '\n' << std::endl;
  }
  // std::cout << "\n";
}
void print_message(Message msg) {
    std::cout << timePointToString(msg.first) << " (user " << msg.second.sender_id << "):\n";
    std::cout << msg.second.content << '\n' << std::endl;

  // std::cout << "\n";
}

constexpr int DEFAULT_PORT = 9090;
constexpr int MAX_EVENTS = 10;
constexpr int BACKLOG = 10;

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
  srv_addr.sin_port = htons(DEFAULT_PORT);

  int opt = 1;
  if (setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setcokopt(SO_REUSEADDR) failed");
  }

  if (bind(srv_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
    perror("Binding failed.");
    exit(EXIT_FAILURE);
  }

  if (listen(srv_sock, BACKLOG) == 0) {
    std::cout << "Server ready. Listening on port " << DEFAULT_PORT << std::endl;
  } else {
    perror("Listening failed.");
  }

  event.events = EPOLLIN;
  event.data.fd = srv_sock;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_sock, &event);

  //chat history via unordered_map
  ChatHistory chat;

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
          auto received_json = json::parse(buffer);
          MessageData msg_data = received_json.get<MessageData>();
          Timepoint now = std::chrono::system_clock::now();
          Message msg{now, msg_data};
          chat.push_back(msg);
          
          print_chat(chat);
        }
      }
    }
  }
  close(epoll_fd);
  close(srv_sock);

  return 0;
}