add_subdirectory(libvsd)

add_executable(vsd main.cpp)
target_link_libraries(vsd libvsd)

install(TARGETS vsd RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
install(FILES vsd.conf DESTINATION bin)
