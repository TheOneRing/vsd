project( VSD )
cmake_minimum_required(VERSION 2.8)

add_definitions(-DUNICODE -D_UNICODE  -D_CRT_RAND_S)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)
if(MINGW)
    add_definitions(-DMINGW_HAS_SECURE_API)
endif(MINGW)

option(STATIC_VSD "Build libvsd static" ON)

if(STATIC_VSD)
    add_definitions(-DLIBVSD_STATIC)
    set(LIBVSD_BUILDTYPE STATIC)

    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS ${MAKE_EXE_LINKER_FLAGS} -static)
    endif(MINGW)
    
    if(MSVC)
        foreach(_bt DEBUG RELEASE RELWITHDEBINFO)
            string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${_bt} ${CMAKE_CXX_FLAGS_${_bt}})
        endforeach(_bt DEBUG RELEASE RELWITHDEBINFO)
    endif(MSVC)
else(STATIC_VSD)
    set(LIBVSD_BUILDTYPE SHARED)
endif(STATIC_VSD)

add_subdirectory(src)
