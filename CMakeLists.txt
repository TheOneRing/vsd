project( VSD )
cmake_minimum_required(VERSION 2.8)

add_definitions(-DUNICODE -D_UNICODE  -D_CRT_RAND_S)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else(MSVC)
    add_definitions(-DMINGW_HAS_SECURE_API)
endif(MSVC)

option(STATIC_VSD "Build libvsd static" ON)

if(${STATIC_VSD})
    add_definitions(-DLIBVSD_STATIC)
    set(LIBVSD_BUILDTYPE STATIC)
else(${STATIC_VSD})
    set(LIBVSD_BUILDTYPE SHARED)
endif(${STATIC_VSD})

add_library(libvsd ${LIBSVSD_BUILDTYPE} vsdprocess.cpp)
set_target_properties( libvsd PROPERTIES DEFINE_SYMBOL BUILDING_LIBVSD)

add_executable(vsd main.cpp )
target_link_libraries(vsd libvsd)

install(TARGETS libvsd vsd RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)