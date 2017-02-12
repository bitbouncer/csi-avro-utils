SET(LIB_SRCS
    hive_schema.h
    hive_schema.cpp
    utils.cpp
    utils.h
    )
add_library (csi-avro-utils STATIC ${LIB_SRCS})