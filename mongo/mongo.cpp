#include "mongo.hpp"

  
std::optional<bsoncxx::document::value> User::get_user_from_db(const std::string& username) {
    mongocxx::database db = MongoManager::get_instance().get_database();
    mongocxx::collection users = db["users"];
    bsoncxx::builder::stream::document user_object;
    user_object << "username" << username;
    auto result = users.find_one(user_object.view());
    if (result) {
      bsoncxx::document::value copy_object_user = bsoncxx::document::value(result->view());
      return copy_object_user;
    }
    return std::nullopt;
}

bool User::add_user(const std::string& username, const std::string& password) {
    
    mongocxx::database db = MongoManager::get_instance().get_database();
    mongocxx::collection users = db["users"];
    bsoncxx::builder::stream::document filter_builder;
    
    filter_builder << "username" << username;
    auto result = users.find_one(filter_builder.view());
    if (!result) {
      bsoncxx::builder::stream::document user_builder;
      user_builder << "username" << username
      << "password_hash" << password
      << "created_at" << bsoncxx::types::b_date{std::chrono::system_clock::now()};
      auto result = users.insert_one(user_builder.view());
      if (result) {
        return true;
      }
      std::cerr << "Error insert document to db" << std::endl;
      
      return false;
    } else {
      return false;
    }
}

bool User::compare_user(const std::string& username, const std::string& password) {
    std::optional<bsoncxx::document::value> user_object = get_user_from_db(username);
    if (!user_object) {
      return false;
    }
    auto view = user_object->view();
    auto it = view.find("password_hash");
    if (it == view.end()) {
      return false;
    }
    std::string current_password = static_cast<std::string>(it->get_string().value);
    if (strcmp(password.data(), current_password.data())) {
      return false;
    }
    return true;
}
    