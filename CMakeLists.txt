cmake_minimum_required(VERSION 2.8)
INCLUDE(${CMAKE_SOURCE_DIR}/../build-scripts/CMakeDependencies.txt)

if(WIN32)
set(EXT_LIBS csi-avro-utils ${CSI_HTTP_CLIENT_LIBS}) 
endif()

if(__LINUX__)
set(EXT_LIBS csi-avro-utils ${CSI_HTTP_CLIENT_LIBS})
endif()

add_subdirectory(csi_avro_utils)
add_subdirectory(programs)
add_subdirectory(tests)


