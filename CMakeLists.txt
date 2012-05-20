project( VSD )
cmake_minimum_required(VERSION 2.8)

add_definitions(-DUNICODE -D_UNICODE )
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_RAND_S)
endif(MSVC)

add_library(libvsd SHARED vsdprocess.cpp)
set_target_properties( libvsd PROPERTIES DEFINE_SYMBOL BUILDING_LIBVSD)



add_executable(vsd main.cpp )
target_link_libraries(vsd libvsd)