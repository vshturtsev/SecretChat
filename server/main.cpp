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

// DataBase includes start
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
// DataBase includes end

#include "../common/message_data.hpp"

// class MessageData;

using json = nlohmann::json;
// using Timepoint = std::chrono::system_clock::time_point;
// using Message = std::pair<Timepoint, MessageData>;
using ChatHistory = std::list<Message>;
using std::cout;

constexpr int BACKLOG = 10;
constexpr int MAX_EVENTS = 10;

using ClientPoll = std::unordered_map<int, ClientSession>;

class ClientSession {
  
 public:
  ClientSession() = default;
  ClientSession(int _fd) : fd(_fd) {}
  const int fd = 0;
  std::string name;
  bool auth_status = false;
};


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

  int handle_client(ClientSession& client_session, ClientPoll& clients_pool) {
    std::vector<uint8_t> bytes_headers = read_from_fd(client_session.fd, 8);
    std::cout << "byte size: " << bytes_headers.size() << " msgHead size: " << sizeof(MsgHeader) << std::endl;
    if (bytes_headers.size() > 0 && bytes_headers.size() != sizeof(MsgHeader))
      return -1;

    if (bytes_headers.size() <= 0) {
      return 0;
    }
    MsgHeader headers = demarshaling_header(bytes_headers);
    std::vector<uint8_t> bytes_message = read_from_fd(client_session.fd, headers.len);
    std::string message = demarshaling_string(bytes_message);
    std::string message_to_broadcast;
    switch (headers.type) {
      case MsgType::Reg:
        reg(client_session, message);
        break;
      case MsgType::Auth:
        auth(client_session, message);
        break;
      
      case MsgType::Chat:
        if (chat_(client_session, message, message_to_broadcast)) {
          broadcast_message(client_session.fd, clients_pool, message_to_broadcast);
        } else {
          cout << "not auth\n";
        }
        break;              
      default:break;
    }
    return 1;
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

void broadcast_message(int autor_fd, ClientPoll clients_pool, std::string message) {
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


class MongoManager {
private:
    mongocxx::instance _instance;
    mongocxx::client _client;
    MongoManager() : _client{mongocxx::uri{"mongodb://root:example@localhost:27017"}} 
        { }
    
    MongoManager(const MongoManager&) = delete;
    MongoManager& operator=(const MongoManager&) = delete;

public:
    static MongoManager& get_instance() {
        static MongoManager instance;
        return instance;
    }

    mongocxx::database get_database(const std::string& name_db = "chat_db") {
        return _client[name_db];
    }
};
  
class User {

public:
  User() = default;
  ClientSession add_user(const std::string& username, const std::string& password) {
    
    mongocxx::database db = MongoManager::get_instance().get_database();
    mongocxx::collection users = db["users"];
    bsoncxx::builder::stream::document filter_builder;
    
    ClientSession new_client;
    filter_builder << "username" << username;
    auto result = users.find_one(filter_builder.view());
    if (!result) {
      bsoncxx::builder::stream::document user_builder;
      user_builder << "username" << username
      << "password_hash" << password
      << "created_at" << bsoncxx::types::b_date{std::chrono::system_clock::now()};
      auto result = users.insert_one(user_builder.view());
      if (result) {
        new_client.auth_status = true;
        new_client.name = username;
        return new_client;
      }
      std::cerr << "Error insert document to db" << std::endl;
      
      return new_client;
    } else {
      return new_client;
    }
  }
  
};

bool reg(ClientSession& client, const std::string& message) {
  AuthMessage auth_data = MessageService::from_string<AuthMessage>(message);
  User user;
  client = user.add_user(auth_data.get_login(), auth_data.get_password());
  return client.auth_status;
}

void auth(ClientSession& client, std::string& message) {
  AuthMessage auth_data = MessageService::from_string<AuthMessage>(message);
  // std::string current_password = get_password_by_username_from_db(auth_data.get_login(), users);
  if (!current_password.empty()) {
    std::cout << "Have such user in db\n";
    if (auth_data.get_password().size() != current_password.size()) {
      std::cout << "Incorrect password\n";
      return;
    }
    if (strcmp(auth_data.get_password().data(), current_password.data())) {
      std::cout << "Incorrect password\n";
      return;

    } else {
      client.auth_status = true;
      client.name = auth_data.get_login();
      std::cout << "Good Auth request(" << message.size() << " bytes): \n" << message << "From: " << client.name << std::endl;
    }

  } else {
    std::cout << "Bad Auth request(" << message.size() << " bytes): \n" << message << std::endl;
  }
}
std::string get_password_by_username_from_db(const std::string& username, mongocxx::collection users) {
  bsoncxx::builder::stream::document filter_builder;
  filter_builder << "username" << username;
  auto result = users.find_one(filter_builder.view());
  
  if (result) {
    auto view = result->view();
    auto it = view.find("password_hash");
    if (it != view.end()) {
      std::string password = static_cast<std::string>(it->get_string().value);
      return password;
    }
  }
  return "";
}

bool chat_(ClientSession& client, std::string& message, std::string& message_to_broadcast) {
  if (client.auth_status) {
    cout << "Received request(" << message.size() << " bytes): \n" << message << std::endl;
    Message restore_msg = MessageService::from_string<Message>(message);
    ChatMessage chat_msg(std::move(restore_msg), client.name);
    chat_msg.update_time();
    message_to_broadcast = MessageService::to_string(chat_msg);
    return true;
  } 
  return false;
}


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
    cout << "client ready: " << ready_fd << "\n";
    for (int i = 0; i < ready_fd; i++) {
      if (ev_list[i].data.fd == serv_fd) {
        int new_sock = accept(server.get_fd(), nullptr, nullptr);
        if (!turn_on_nonblock(new_sock)) {
          close(new_sock);
          continue;
        }
        
        cout << "connected: " << new_sock << "\n";
        clients_pool.emplace(new_sock, ClientSession{new_sock});
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = new_sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);

      } else {
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
          } else if (status > 0){
            perror("failed handle client");
            close(fd);
          }
        }
      }
    }
  }
  return 0;
}