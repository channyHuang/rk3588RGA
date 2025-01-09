cmake_minimum_required(VERSION 3.10)

set(v5 False)
set(Lib True)

if (v5)
project(HGRknnDetect_v5 VERSION 1.0.0)
add_definitions(-DENABLE_V5)
else()
project(HGRknnDetect_v8 VERSION 1.0.0)
endif()

if (Lib)
else()
find_package(OpenCV REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils)

include_directories(${PROJECT_NAME}
${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/librga/include/
${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rknpu2/include/
)

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
if (v5)
file(GLOB FILES_v ${CMAKE_CURRENT_SOURCE_DIR}/yolov5/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/yolov5/*.cc )
else()
file(GLOB FILES_v ${CMAKE_CURRENT_SOURCE_DIR}/yolov8/*.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/yolov8/*.cc )
endif()
file(GLOB FILES 
${CMAKE_CURRENT_SOURCE_DIR}/YoloManager.cpp 
${CMAKE_CURRENT_SOURCE_DIR}/Detector.cpp 
${CMAKE_CURRENT_SOURCE_DIR}/image_rga.cpp 
${CMAKE_CURRENT_SOURCE_DIR}/RknnPool.cpp 
${CMAKE_CURRENT_SOURCE_DIR}/libExport.cpp 
${CMAKE_CURRENT_SOURCE_DIR}/ThreadPool.cpp )

if (Lib)
add_library(${PROJECT_NAME} SHARED
${HEADERS}
${FILES}
${FILES_v}
)
else()
add_executable(${PROJECT_NAME}
${HEADERS}
${FILES}
${FILES_v}
main.cpp
)
endif()


target_link_directories(${PROJECT_NAME} PUBLIC
  ${LIB_PATH}
)

if (Lib)
target_link_libraries(${PROJECT_NAME}
utils
rga
rknnrt
pthread
)
else()
target_link_libraries(${PROJECT_NAME}
utils
rga
rknnrt
pthread
${OpenCV_LIBS}
)
endif()

# set_target_properties(${PROJECT_NAME} 
                      # PROPERTIES 
                      # VERSION 1.0.0 )
                      # SOVERSION 1 )    
