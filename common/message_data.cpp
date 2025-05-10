#include "message_data.hpp"


#include <arpa/inet.h>
// #include <cstring>
#include <nlohmann/json.hpp>
#include <string>


bytes Request::marshaling() {
  bytes data;
  data.push_back(static_cast<uint8_t>(type));
  data.insert(data.end(), body.begin(), body.end());
  return data;
}

int Request::demarshaling(bytes&& data) {
  if (data.size() < 1) return -1;
  type = static_cast<ReqType>(*data.begin());
  body.insert(body.end(), (data.begin() + sizeof(type)), data.end());
  return 0;
}

std::ostringstream ChatMessage::get_display_view()  {
  std::ostringstream oss;
  oss << sender_name << " "
      << std::put_time(std::localtime(&msg_time), "%Y-%m-%d %H:%M:%S:\n")
      << content; // << std::endl;
  return oss;
}

// std::string encode(const std::string &str) {}
