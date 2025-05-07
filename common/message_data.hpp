#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <cstring>
#include <arpa/inet.h>

using json = nlohmann::json;

enum class MsgType {
  Auth, // authentification
  Chat, // message to chat
  Reg,  // registration, first time
  Ping  //?need this? check connection
};

struct MsgHeader {
  MsgType type;
  uint32_t len;
};

std::vector<uint8_t> marshaling(MsgHeader& headers) {
  uint32_t net_type = htonl(static_cast<uint32_t>(headers.type));
  uint32_t net_len = htonl(headers.len);
  std::vector<uint8_t> bytes;
  bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&headers.type),
                            reinterpret_cast<uint8_t *>(&headers.type) + sizeof(headers.type));

  bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&headers.len),
               reinterpret_cast<uint8_t *>(&headers.len) + sizeof(headers.len));
  return bytes;
}

MsgHeader demarshaling_header(std::vector<uint8_t>& bytes) {
  MsgHeader headers = {};
  memcpy(&headers.type, bytes.data(), sizeof(headers.type));
  memcpy(&headers.len, bytes.data() + sizeof(headers.type), sizeof(headers.len));
  return headers;
}

std::string demarshaling_string(std::vector<uint8_t>& bytes) {
  return std::string(bytes.begin(), bytes.end());
}

class AuthMessage {
protected:
  std::string login;
  std::string password;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(AuthMessage, login, password);
  AuthMessage() = default;
  AuthMessage(std::string& login, std::string& password) : login(login), password(password)
    { }
    
  const std::string& get_login() const { return login; }
  const std::string& get_password() const {return password; }
};

class Message {
protected:
  MsgType type;
  std::string content;
  time_t msg_time;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, type, content, msg_time);
  Message() = default;
  // Message(Message&&) = default; 
  Message(MsgType _type, std::string _content)
      : type(_type), content(_content) {
    time(&msg_time);
  }
  const MsgType &get_type() const { return type; }
  const std::string &get_content() const { return content; }
  const time_t &get_time() const { return msg_time; }
  void update_time() {
    time(&msg_time);
  }
};

class ChatMessage : public Message {
protected:
  std::string sender_name;

public:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(ChatMessage, type, content, msg_time, sender_name);
  ChatMessage() = default;
  ChatMessage(Message &&base_msg) : Message(std::move(base_msg)) {}
  ChatMessage(Message &&base_msg, const std::string &_name)
      : Message(std::move(base_msg)), sender_name(_name) {}

  const std::string &get_sender_name() const { return sender_name; }
  void set_sender_name(std::string name) { sender_name = name; }

  std::ostringstream get_display_view() {
    std::ostringstream oss;
    oss << sender_name << " "
        << std::put_time(std::localtime(&msg_time), "%Y-%m-%d %H:%M:%S:\n")
        << content;// << std::endl;
    return oss;
  }
};

namespace MessageService {
template <typename T> std::string to_string(const T &msg) {
  std::string json_str = json(msg).dump();
  return json_str;
}

template <typename T> T from_string(const std::string &str) {
  try {
    auto received_json = json::parse(str);
    return received_json.get<T>();
  } catch (const json::exception &e) {
    throw std::runtime_error("JSON error: " + std::string(e.what()));
  }
}
// std::string encode(const std::string &str) {}

// std::string decode(const std::string &str) {}
} 
