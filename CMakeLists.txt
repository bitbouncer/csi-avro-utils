cmake_minimum_required(VERSION 2.8.11)

if(WIN32)
SET(CSI_INCLUDE_PATH
  ${CMAKE_SOURCE_DIR}/../boost
  ${CMAKE_SOURCE_DIR}/../zlib
  ${CMAKE_SOURCE_DIR}/../zlib/build
  ${CMAKE_SOURCE_DIR}/../openssl/include
  ${CMAKE_SOURCE_DIR}/../avro/lang/c++/include
)

SET(CSI_LIBRARY_PATH        
  ${CMAKE_SOURCE_DIR}/../boost/stage/lib/$(Platform)/lib
  ${CMAKE_SOURCE_DIR}/../openssl
  ${CMAKE_SOURCE_DIR}/../zlib/build
)

SET(LIBRARY_OUTPUT_PATH     ${CMAKE_SOURCE_DIR}/lib/$(Platform))
SET(EXECUTABLE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin/$(Platform))

add_definitions(-D_FILE_OFFSET_BITS=64 -D_REENTRANT -DEXTERNAL_LOCKS -DMULTITHREAD)
add_definitions(-DBOOST_ASIO_HAS_STD_CHRONO)
add_definitions(-DMARCH_x86_64) 
add_definitions(-D_WIN32_WINNT=0x0602) # win8.1 to get nthll 64 bit
add_definitions(-DWINVER=0x0601)
add_definitions(-DCF_WINDOWS)
add_definitions(/MP)
add_definitions(/bigobj)
add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DSECURITY_WIN32)
add_definitions(/wd4996) #'strdup': The POSIX name for this item is deprecated
add_definitions(/wd4197) # 'cf_atomic32' : top-level volatile in cast is ignored
add_definitions(/wd4200) #nonstandard extension used : zero-sized array in struct/union 
add_definitions(-DLIBRDKAFKA_STATICLIB)
add_definitions(-DBOOST_ALL_STATIC_LINK)

SET(EXT_LIBS csi-avro-utils csi-avro ${CSI_BOOST_LIBS} libssl libcrypto ${ZLIB_LIBS} crypt32 Ws2_32 legacy_stdio_definitions)

elseif(APPLE)
add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-std=c++11)

set(LIBRARY_OUTPUT_PATH     ${CMAKE_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin)

set(CSI_BOOST_LIBS boost_log_setup boost_log-mt boost_date_time boost_timer boost_thread-mt boost_system boost_program_options boost_filesystem boost_regex boost_chrono boost_iostreams pthread c)
SET(EXT_LIBS csi-avro-utils avro ${CSI_BOOST_LIBS} crypto ssl)

elseif(ALPINE_LINUX)

set(LIBRARY_OUTPUT_PATH     ${CMAKE_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin)

add_definitions(-D_FILE_OFFSET_BITS=64 -D_REENTRANT -DEXTERNAL_LOCKS -DMULTITHREAD)
add_definitions(-fPIC)
add_definitions(-std=c++11)
add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-Wno-deprecated)

#boost_thread
set(CSI_BOOST_LIBS boost_log_setup boost_log-mt boost_date_time boost_timer boost_system boost_program_options boost_filesystem boost_regex boost_chrono boost_iostreams pthread rt c)
SET(EXT_LIBS csi-avro-utils avro ${CSI_BOOST_LIBS} crypto ssl)
else()
#LINUX
set(LIBRARY_OUTPUT_PATH     ${CMAKE_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin)

add_definitions(-D_FILE_OFFSET_BITS=64 -D_REENTRANT -DEXTERNAL_LOCKS -DMULTITHREAD)
add_definitions(-fPIC)
add_definitions(-std=c++11)
add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-Wno-deprecated)

#boost_thread
set(CSI_BOOST_LIBS boost_log_setup boost_log boost_date_time boost_timer boost_system boost_program_options boost_filesystem boost_regex boost_chrono boost_iostreams pthread rt c)
SET(EXT_LIBS csi-avro-utils avro ${CSI_BOOST_LIBS} crypto ssl)
endif() 

if(WIN32)
#dirty fix to complile non supported unicode static lib on windows...
#AVRO
SET (AVROCPP_PATH ${CMAKE_SOURCE_DIR}/../avro/lang/c++)
file(GLOB CSI_AVRO_CC
    ${AVROCPP_PATH}/impl/*.cc
	${AVROCPP_PATH}/impl/json/*.cc
	${AVROCPP_PATH}/impl/parsing/*.cc
)
file(GLOB CSI_AVRO_H ${AVROCPP_PATH}/api/*.hh)
file(GLOB CSI_AVRO_BUFFER_H ${AVROCPP_PATH}/api/buffer/*.hh)
file(MAKE_DIRECTORY ${AVROCPP_PATH}/include)
file(MAKE_DIRECTORY ${AVROCPP_PATH}/include/avro)
file(MAKE_DIRECTORY ${AVROCPP_PATH}/include/avro/buffer)
file(COPY ${CSI_AVRO_H} DESTINATION ${AVROCPP_PATH}/include/avro)
file(COPY ${CSI_AVRO_BUFFER_H} DESTINATION ${AVROCPP_PATH}/include/avro/buffer)
ADD_LIBRARY(csi-avro STATIC ${CSI_AVRO_CC} ${CSI_AVRO_H} ${CSI_AVRO_BUFFER_H})
TARGET_INCLUDE_DIRECTORIES(csi-avro PRIVATE ${AVROCPP_PATH}/api ${CSI_INCLUDE_PATH})
endif()

include_directories(${CSI_INCLUDE_PATH} ${CMAKE_SOURCE_DIR})
link_directories(${CSI_LIBRARY_PATH})

add_subdirectory(csi_avro_utils)
add_subdirectory(programs)
