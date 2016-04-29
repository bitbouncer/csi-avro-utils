#include <boost/make_shared.hpp>
#include "hive_schema.h"
#include <avro/Generic.hh>
#include <avro/Schema.hh>

namespace csi {
  namespace avro_hive {
    boost::shared_ptr<avro::Schema> create_hive_column_schema(avro::Type t) {
      //AVRO_STRING,    /*!< String */
      //    AVRO_BYTES,     /*!< Sequence of variable length bytes data */
      //    AVRO_INT,       /*!< 32-bit integer */
      //    
      //    ,     /*!< Floating point number */
      //    AVRO_DOUBLE,    /*!< Double precision floating point number */
      //    AVRO_BOOL,      /*!< Boolean value */
      //    AVRO_NULL,      /*!< Null */

      //    AVRO_RECORD,    /*!< Record, a sequence of fields */
      //    AVRO_ENUM,      /*!< Enumeration */
      //    AVRO_ARRAY,     /*!< Homogeneous array of some specific type */
      //    AVRO_MAP,       /*!< Homogeneous map from string to some specific type */
      //    AVRO_UNION,     /*!< Union of one or more types */
      //    AVRO_FIXED,     /*!< Fixed number of bytes */

      boost::shared_ptr<avro::Schema>  value_schema;
      switch(t) {
      case avro::AVRO_INT:
      value_schema = boost::make_shared<avro::IntSchema>();
      break;
      case avro::AVRO_LONG:
      value_schema = boost::make_shared<avro::LongSchema>();
      break;
      default:
      value_schema = boost::make_shared<avro::StringSchema>();
      break;
      };

      /* Make a union of value_schema with null. Some types are already a union,
      * in which case they must include null as the first branch of the union,
      * and return directly from the function without getting here (otherwise
      * we'd get a union inside a union, which is not valid Avro). */
      boost::shared_ptr<avro::Schema> null_schema = boost::make_shared<avro::NullSchema>();
      boost::shared_ptr<avro::UnionSchema> union_schema = boost::make_shared<avro::UnionSchema>();
      union_schema->addType(*null_schema);
      union_schema->addType(*value_schema);
      return union_schema;
    }


    boost::shared_ptr<avro::Schema> create_hive_strict_key_schema(avro::Type t) {
      //AVRO_STRING,    /*!< String */
      //    AVRO_BYTES,     /*!< Sequence of variable length bytes data */
      //    AVRO_INT,       /*!< 32-bit integer */
      //    
      //    ,     /*!< Floating point number */
      //    AVRO_DOUBLE,    /*!< Double precision floating point number */
      //    AVRO_BOOL,      /*!< Boolean value */
      //    AVRO_NULL,      /*!< Null */

      //    AVRO_RECORD,    /*!< Record, a sequence of fields */
      //    AVRO_ENUM,      /*!< Enumeration */
      //    AVRO_ARRAY,     /*!< Homogeneous array of some specific type */
      //    AVRO_MAP,       /*!< Homogeneous map from string to some specific type */
      //    AVRO_UNION,     /*!< Union of one or more types */
      //    AVRO_FIXED,     /*!< Fixed number of bytes */

      boost::shared_ptr<avro::Schema>  value_schema;
      switch(t) {
      case avro::AVRO_INT:
      return boost::make_shared<avro::IntSchema>();
      break;
      case avro::AVRO_LONG:
      return boost::make_shared<avro::LongSchema>();
      break;
      default:
      return boost::make_shared<avro::StringSchema>();
      break;
      };
    }

    boost::shared_ptr<avro::ValidSchema>  get_key_schema(const avro::Name& schema_name, const std::vector<std::string>& keys, bool allow_null, const avro::ValidSchema& src_schema) {
      auto r = src_schema.root();
      assert(r->type() == avro::AVRO_RECORD);
      size_t nr_of_leaves = r->leaves();

      boost::shared_ptr<avro::RecordSchema> key_schema = boost::make_shared<avro::RecordSchema>(schema_name.fullname());
      for(std::vector<std::string>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        for(size_t j = 0; j != nr_of_leaves; ++j) {
          auto name = r->nameAt(j);
          if(name == *i) {
            auto l = r->leafAt(j);
            if(allow_null)
              key_schema->addField(*i, *create_hive_column_schema(l->type()));
            else
              key_schema->addField(*i, *create_hive_strict_key_schema(l->type()));
            break;
          }
        }
      }
      return boost::make_shared<avro::ValidSchema>(*key_schema);
    }

    boost::shared_ptr<avro::ValidSchema> get_value_schema(const std::string& schema_name, const avro::ValidSchema& src_schema, const avro::ValidSchema& key_schema) {
      auto r = src_schema.root();
      assert(r->type() == avro::AVRO_RECORD);
      boost::shared_ptr<avro::RecordSchema> value_schema = boost::make_shared<avro::RecordSchema>(schema_name);
      return boost::make_shared<avro::ValidSchema>(*value_schema);
    }

    boost::shared_ptr<avro::GenericDatum> get_key(avro::GenericDatum& value_datum, const avro::ValidSchema& key_schema) {
      boost::shared_ptr<avro::GenericDatum> key = boost::make_shared<avro::GenericDatum>(key_schema);
      assert(key_schema.root()->type() == avro::AVRO_RECORD);
      assert(value_datum.type() == avro::AVRO_RECORD);
      size_t nKeyFields = key_schema.root()->leaves();

      avro::GenericRecord& key_record(key->value<avro::GenericRecord>());
      avro::GenericRecord& value_record(value_datum.value<avro::GenericRecord>());

      //size_t nValueFields = value_record.fieldCount();

      for(int i = 0; i < nKeyFields; i++) {
        std::string column_name = key_schema.root()->nameAt(i);
        assert(value_record.hasField(column_name));
        assert(key_record.hasField(column_name));
        // we need to handle keys with or without nulls ie union or value
        if(key_record.field(column_name).type() == avro::AVRO_UNION) {
          key_record.field(column_name) = value_record.field(column_name);
        } else {
          avro::GenericUnion& au(value_record.field(column_name).value<avro::GenericUnion>());
          avro::GenericDatum& actual_column_value = au.datum();
          key_record.field(column_name) = actual_column_value;
        }
      }
      return key;
    }
  };
};