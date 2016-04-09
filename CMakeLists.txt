cmake_minimum_required(VERSION 2.8.1)
cmake_policy(SET CMP0015 NEW)

include_directories(${CSI_INCLUDE_PATH} ${CMAKE_SOURCE_DIR})
link_directories(${CSI_LIBRARY_PATH})

if(WIN32)
set(EXT_LIBS csi-avro-utils ${CSI_HTTP_CLIENT_LIBS}) 
endif()

if(__LINUX__)
set(EXT_LIBS csi-avro-utils ${CSI_HTTP_CLIENT_LIBS})
endif()

add_subdirectory(csi_avro_utils)
add_subdirectory(programs)
add_subdirectory(tests)


