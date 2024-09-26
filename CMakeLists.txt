cmake_minimum_required(VERSION 3.11)
project( VSD )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(ECM 5.50.0 QUIET NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
    include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
else()
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -utf-8)
    endif(MSVC)
    if(MINGW)
        add_definitions(-DMINGW_HAS_SECURE_API -D_WIN32_WINNT=0x0600)
    endif(MINGW)
endif()

if(MSVC)
    # treat unhandled switch cases as error
    add_compile_options(/we4062)
endif ()

option(STATIC_VSD "Build libvsd static" ON)

if(STATIC_VSD)
    add_definitions(-DLIBVSD_STATIC)
    set(LIBVSD_BUILDTYPE STATIC)

    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS ${MAKE_EXE_LINKER_FLAGS} -static )
    endif(MINGW)
    
    if(MSVC)
        # Use static runtime for all subdirectories
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif(MSVC)
else(STATIC_VSD)
    set(LIBVSD_BUILDTYPE SHARED)
endif(STATIC_VSD)
message(STATUS "LIBVSD_BUILDTYPE=${LIBVSD_BUILDTYPE}")

add_subdirectory(src)
add_subdirectory(test)
