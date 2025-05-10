#pragma once

// DataBase includes start
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
// DataBase includes end


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
    std::optional<bsoncxx::document::value> get_user_from_db(const std::string& username);
  
public:
  User() = default;
  bool add_user(const std::string& username, const std::string& password);
  bool compare_user(const std::string& username, const std::string& password);
  
};
