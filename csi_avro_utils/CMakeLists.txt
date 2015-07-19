SET(LIB_SRCS
    confluent_schema_registry.h
    confluent_schema_registry.cpp
	confluent_codec.h
	confluent_codec.cpp
	)
add_library (csi-avro-utils STATIC ${LIB_SRCS})