cmake_minimum_required(VERSION 3.10)

project(Mython CXX)
set(CMAKE_CXX_STANDARD 17)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
# set(MAIN_FILE ${SRC_DIR}/main.cpp)


# ${MAIN_FILE} должно устанавливаться -D аргументом при build
add_executable(${PROJECT_NAME} ${MAIN_FILE} ${SRC_DIR}/lexer.cpp ${SRC_DIR}/lexer_test_open.cpp)
