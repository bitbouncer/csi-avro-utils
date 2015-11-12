SET(LIB_SRCS
    avro_fingerprint_encoding.h
    avro_raw_encoding.h
    avro_schema_id_encoding.h
    confluent_schema_registry.h
    confluent_schema_registry.cpp
	confluent_codec.h
	confluent_codec.cpp
    hive_schema.h
    hive_schema.cpp
    utils.cpp
    utils.h
    )
add_library (csi-avro-utils STATIC ${LIB_SRCS})