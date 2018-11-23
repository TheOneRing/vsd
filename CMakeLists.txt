cmake_minimum_required(VERSION 3.11)
project( VSD )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_definitions(-DUNICODE -D_UNICODE  -D_CRT_RAND_S -D_WIN32_WINNT=0x0600)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -utf-8 )
endif(MSVC)
if(MINGW)
    add_definitions(-DMINGW_HAS_SECURE_API)
endif(MINGW)

option(STATIC_VSD "Build libvsd static" OFF)

if(STATIC_VSD)
    add_definitions(-DLIBVSD_STATIC)
    set(LIBVSD_BUILDTYPE STATIC)

    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS ${MAKE_EXE_LINKER_FLAGS} -static )
    endif(MINGW)
    
    if(MSVC)
        foreach(_bt DEBUG RELEASE RELWITHDEBINFO)
            string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${_bt} ${CMAKE_CXX_FLAGS_${_bt}})
        endforeach(_bt DEBUG RELEASE RELWITHDEBINFO)
    endif(MSVC)
else(STATIC_VSD)
    set(LIBVSD_BUILDTYPE SHARED)
endif(STATIC_VSD)
message(STATUS "LIBVSD_BUILDTYPE=${LIBVSD_BUILDTYPE}")

add_subdirectory(src)
add_subdirectory(test)
