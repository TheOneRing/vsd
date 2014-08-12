include_directories(${CMAKE_CURRENT_BINARY_DIR}/libvsd)

add_subdirectory(libvsd)

add_executable(vsd main.cpp )
target_link_libraries(vsd libvsd)

install(TARGETS vsd RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)