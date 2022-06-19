include(GenerateExportHeader)

add_library(libvsd_gflags STATIC ${PROJECT_SOURCE_DIR}/src/3dparty/ceee/gflag_utils.cc)
target_link_libraries(libvsd_gflags PUBLIC ntdll)

add_library(libvsd ${LIBVSD_BUILDTYPE} vsdprocess.cpp vsdchildprocess.cpp utils.cpp)
target_link_libraries(libvsd PUBLIC shlwapi libvsd_gflags psapi)

generate_export_header(libvsd 
  EXPORT_FILE_NAME vsd_exports.h
  EXPORT_MACRO_NAME LIBVSD_EXPORT
)

target_include_directories(libvsd PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                                         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/>)
install(TARGETS libvsd RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
                     
install(FILES vsdprocess.h ${CMAKE_CURRENT_BINARY_DIR}/vsd_exports.h DESTINATION include/vsd)
