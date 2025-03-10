cmake_minimum_required(VERSION 3.21)
set(projectName AliyunDDNS)
project(${projectName})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

link_directories(libs)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)

include_directories(include)

add_executable(
        ${projectName}

        main.cpp

        SMTP/SMTP.cpp
        Logger/Logger.cpp

        DDNS/DDNS.cpp
        DDNS/DDNS_config_.cpp
        DDNS/DDNS_SMTP.cpp
        DDNS/DDNS_GetIP.cpp
        DDNS/DDNS_updateDNS.cpp
)

target_link_libraries(
        ${projectName}
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
)
