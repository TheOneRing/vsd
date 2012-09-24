add_library(libvsd ${LIBSVSD_BUILDTYPE} vsdprocess.cpp vsdchildprocess.cpp)
set_target_properties( libvsd PROPERTIES DEFINE_SYMBOL BUILDING_LIBVSD)

install(TARGETS libvsd RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
                     
install(FILES vsdprocess.h DESTINATION include/vsd)
