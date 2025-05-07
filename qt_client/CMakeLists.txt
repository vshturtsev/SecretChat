cmake_minimum_required(VERSION 3.5)

project(Qt_ClientApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Core REQUIRED)
# find_package(Qt6 COMPONENTS Widgets REQUIRED)

set (PROJECT_SOURCES 
        main.cpp        
)

add_executable(ClientApp ${PROJECT_SOURCES})

target_link_libraries(ClientApp PRIVATE Qt6::Widgets Qt6::Core)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ClientApp)
endif()



