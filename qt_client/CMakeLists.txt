cmake_minimum_required(VERSION 3.5)

project(Qt_ClientApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#для QtCreator
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Core REQUIRED)
# find_package(Qt6 COMPONENTS Widgets REQUIRED)


# set(COMMON_SOURCES 
#     ../common/message_data.cpp  # .cpp файл
#     ../common/message_data.hpp  # .hpp файл
# )
include_directories(../common)

set(SOURCES_ClientApp
        ../common/message_data.cpp
        ../common/socket.cpp
        main.cpp
        src/ChatListService.cpp  
        src/ChatListView.cpp  
        src/ChatService.cpp  
        src/ChatView.cpp  
        src/LoginService.cpp  
        src/LoginView.cpp               
)

set(HEADERS_ClientApp
        ../common/socket.hpp
        ../common/message_data.hpp
        include/ChatListService.hpp  
        include/ChatListView.hpp  
        include/ChatService.hpp  
        include/ChatView.hpp  
        include/LoginService.hpp  
        include/LoginView.hpp
)

#TestServer build, to run server and client: bash run.sh
file(COPY run.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(SOURCES_TestServer 
        ../common/message_data.cpp   
        ../common/socket.cpp
        test_server.cpp     
) 

set(HEADERS_TestServer
        ../common/message_data.hpp
        ../common/socket.hpp
)

add_executable(TestServer ${SOURCES_TestServer} ${HEADERS_TestServer})

add_executable(ClientApp ${SOURCES_ClientApp} ${HEADERS_ClientApp})

target_link_libraries(ClientApp PRIVATE Qt6::Widgets Qt6::Core)

target_include_directories(ClientApp PRIVATE include)

# Включение MOC (для сигналов/слотов)
set_target_properties(ClientApp PROPERTIES
    AUTOMOC ON
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ClientApp)
endif()



