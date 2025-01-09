cmake_minimum_required(VERSION 3.10)

project(utils LANGUAGES C CXX)

# set(CMAKE_CXX_STANDARD 17)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/)

include_directories(${PROJECT_NAME}
${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/librga/include/
${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/rknpu2/include/
)

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_definitions(-DLIBRGA_IM2D_HANDLE)

add_library(${PROJECT_NAME}
    ${HEADERS}
    ${FILES}
)

target_link_directories(${PROJECT_NAME} PUBLIC
  ${LIB_PATH}
)

target_link_libraries(${PROJECT_NAME}
rknnrt
rga
)