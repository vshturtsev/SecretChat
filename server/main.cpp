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

#include "../common/message_data.hpp"

// class MessageData;

using json = nlohmann::json;
// using Timepoint = std::chrono::system_clock::time_point;
// using Message = std::pair<Timepoint, MessageData>;
using ChatHistory = std::list<Message>;
using std::cout;



// void print_chat(ChatHistory chat) {
//   std::cout << "Chat History:\n";
//   for (auto it = chat.begin(); it != chat.end(); ++it) {
//     std::cout << timePointToString(it->first) << " (user " << it->second.sender_id << "):\n";
//     std::cout << it->second.content << '\n' << std::endl;
//   }
// }

// void proccess_message(const char *buffer, ChatHistory &chat) {
//   auto received_json = json::parse(buffer);
//   MessageData msg_data = received_json.get<MessageData>();
//   Timepoint now = std::chrono::system_clock::now();
//   Message msg{now, msg_data};
//   chat.push_back(msg);
//   print_chat(chat);
// }

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

void broadcast_message(int autor_fd, std::unordered_map<int, ClientSession> clients_pool, std::string message) {
  // std::cout << "broadcast: " << message << std::endl;
  for (const auto &[fd, client_session] : clients_pool) {
    if (fd != autor_fd && client_session.auth_status) {
      if (send(fd, message.data(), message.size(), 0) < 0) {
        perror("Error send message.");
      }
    }
  }
}

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

std::vector<uint8_t> read_from_fd(int fd, ssize_t length) {
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
    std::cout << recived_bytes << "\n";
    bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + recived_bytes);
  }

  return bytes;
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
    cout << "client ready: " << ready_fd << "\n";
    for (int i = 0; i < ready_fd; i++) {
      if (ev_list[i].data.fd == serv_fd) {
        int new_sock = accept(server.get_fd(), nullptr, nullptr);
        if (!turn_on_nonblock(new_sock)) {
          close(new_sock);
          continue;
        }
        
        cout << "connected: " << new_sock << "\n";
        clients_pool.emplace(new_sock, ClientSession{"", false});
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = new_sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);

      } else {
        int fd = ev_list[i].data.fd;
        cout << "client session: " << fd << "\n";        
        auto client = clients_pool.find(fd);
        if (client != clients_pool.end()) {          
          // ssize_t receive_headers = recv(client->first, &headers, sizeof(headers), MSG_WAITALL);
          std::vector<uint8_t> bytes_headers = read_from_fd(client->first, 8);
          std::cout << "byte size: " << bytes_headers.size() << " msgHead size: " << sizeof(MsgHeader) << std::endl;
          if (bytes_headers.size() > 0 && bytes_headers.size() != sizeof(MsgHeader)) {
            perror("failed read headers");
            continue;
          }
          if (bytes_headers.size() <= 0) {
            cout << "disconnect client\n";
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->first, &event);
            clients_pool.erase(client->first);
            close(fd);       
          } else {           
            MsgHeader headers = demarshaling(bytes_headers);
            std::vector<uint8_t> bytes_message = read_from_fd(client->first, headers.len);
            std::string msg = demarshaling_string(bytes_message);
            switch (headers.type) {
              case MsgType::Reg:
                break;

              case MsgType::Auth:
                client->second.auth_status = true;
                client->second.name = "CLIENT_NAME_TODO";//TODO
                std::cout << "Auth request(" << bytes_message.size() << " bytes): \n" << msg << std::endl;
                break;
              
              case MsgType::Chat:
                if (client->second.auth_status) {
                  cout << "Received request(" << bytes_message.size() << " bytes): \n" << msg << std::endl;
                  Message restore_msg = MessageService::from_string<Message>(msg);
                  ChatMessage chat_msg(std::move(restore_msg), client->second.name);
                  // ChatMessage chat_msg(std::move(restore_msg), "CLIENT_NAME");
                  chat_msg.update_time();
                  // cout << chat_msg.get_display_view().str() << "\n"; // TEST
                  std::string new_msg = MessageService::to_string(chat_msg);
                  // cout << new_msg << std::endl;
                  broadcast_message(client->first, clients_pool, new_msg);
                } else {
                  cout << "not auth\n";
                }
                break;              
              default:break;
            }
          }
        }
      }
    }
  }
  return 0;
}