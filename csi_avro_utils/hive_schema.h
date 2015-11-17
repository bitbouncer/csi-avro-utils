#pragma once
#include <avro/ValidSchema.hh>

namespace csi
{
    namespace avro_hive
    {
        boost::shared_ptr<avro::ValidSchema>  get_key_schema(const avro::Name& key_schema_name, const std::vector<std::string>& keys, bool allow_null, const avro::ValidSchema& value_schema);
        boost::shared_ptr<avro::GenericDatum> get_key(avro::GenericDatum& value_datum, const avro::ValidSchema& key_schema);
    };
};

