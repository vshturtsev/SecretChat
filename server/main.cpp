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
#include <unordered_set>

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
}

void print_message(Message msg) {
    std::cout << timePointToString(msg.first) << " (user " << msg.second.sender_id << "):\n";
    std::cout << msg.second.content << '\n' << std::endl;
}

void proccess_message(const char *buffer, ChatHistory& chat) {
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
        if (fd < 0){
            perror("Failed error create socket");
            exit(EXIT_FAILURE);
        }
    }
    ~Server() {
        close(fd);
    }

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

class ClientPool {
    int epoll_fd;
    std::unordered_set<int> clients;
    struct epoll_event event, ev_list[MAX_EVENTS];

public:
    ClientPool(int serv_fd) {
        epoll_fd = epoll_create1(0);
        event.events = EPOLLIN;
        event.data.fd = serv_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serv_fd, &event);
    }
    ~ClientPool() {
        close(epoll_fd);
    }

    void add_event(int fd) {
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    }

    int get_ready_events(void) {
        return epoll_wait(epoll_fd, ev_list, MAX_EVENTS, -1);
    }

    void delete_event(int index) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev_list[index].data.fd, &event);
    }

    void append_client(int fd) {
        clients.insert(fd);
    }

    void delete_client(int index) {
        clients.erase(ev_list[index].data.fd);
    }

    void broadcast_message(int index_of_author, char *msg) {
        for (int client_fd : clients) {
            if (ev_list[index_of_author].data.fd != client_fd) {
                if (send(client_fd, msg, strlen(msg), 0) < 0) {
                    perror("Error send message.");
                }
            }
        }
    }

    struct epoll_event& get_item_by_index(int index) {
        return ev_list[index];
    }
};

int main(void) {
    // if (fopen("config.txt"))
    // else {
    //     take_args(); // scanf();
    // }
    Server server;
    server.start_listen(9090);
    ClientPool pool(server.get_fd());
    ChatHistory chat;
    while (true) {
        int ready_fd = pool.get_ready_events();
        for (int i = 0; i < ready_fd; i++) {
            if (pool.get_item_by_index(i).data.fd == server.get_fd()) {
                int new_sock = accept(server.get_fd(), nullptr, nullptr);
                pool.add_event(new_sock);
                pool.append_client(new_sock);
            } else {
                char buffer[2048];
                int bytes = recv(pool.get_item_by_index(i).data.fd, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    pool.delete_event(i);
                    pool.delete_client(i);
                
                } else {
                    std::cout << "Received request(" << bytes << " bytes): \n"
                    << buffer << std::endl;
                    pool.broadcast_message(i, buffer);
                    // proccess_message(buffer, chat);
                }
            }
        }
    }
    return 0;
}