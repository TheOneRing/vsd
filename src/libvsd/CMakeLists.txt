include(GenerateExportHeader)

add_library(libvsd ${LIBSVSD_BUILDTYPE} vsdprocess.cpp vsdchildprocess.cpp)
target_link_libraries(libvsd shlwapi)

generate_export_header(libvsd 
  EXPORT_FILE_NAME vsd_exports.h
  EXPORT_MACRO_NAME LIBVSD_EXPORT
)

install(TARGETS libvsd RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
                     
install(FILES vsdprocess.h vsd_exports.h DESTINATION include/vsd)
