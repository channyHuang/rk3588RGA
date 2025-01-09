cmake_minimum_required(VERSION 3.10)

set(Lib False)

project(HGStream VERSION 1.0.0)

if (Lib)
else()
find_package(OpenCV REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/)

include_directories(${PROJECT_NAME}
${CMAKE_CURRENT_SOURCE_DIR}/include/
)

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/*.cc )

if (Lib)
add_library(${PROJECT_NAME} SHARED
${HEADERS}
${FILES}
)
else()
add_executable(${PROJECT_NAME}
${HEADERS}
${FILES}
)
endif()

target_link_directories(${PROJECT_NAME} PUBLIC
  ${LIB_PATH}
)

if (Lib)
target_link_libraries(${PROJECT_NAME}
ff_media
pthread
)
else()
target_link_libraries(${PROJECT_NAME}
ff_media
pthread
${OpenCV_LIBS}
)
endif()

set_target_properties(${PROJECT_NAME} 
                      PROPERTIES 
                      VERSION 1.0.0 )
                      # SOVERSION 1 )    
