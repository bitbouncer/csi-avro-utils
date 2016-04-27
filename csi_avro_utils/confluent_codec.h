#include "confluent_schema_registry.h"
#pragma once


/*
    encoding format
    byte0   -> 0 (magic)
    byte1-4 -> big endian encoded shema id
    byte5-N -> avro encoded message
    */

namespace confluent {
  class codec {
  public:
    enum error_code_t { SUCCESS = 0, WOULD_BLOCK = 1, NOT_FOUND = 2, NO_CONNECTION = 3, INTERNAL_SERVER_ERROR = 4, NOT_AVRO = 5 };
    enum { CONFLUENT_MAGIC_BYTE = 0x0 };

    static std::string to_string(error_code_t);

    struct decode_result {
      decode_result() : ec(SUCCESS) {}
      decode_result(int32_t ec_, boost::shared_ptr<avro::ValidSchema> s, boost::shared_ptr<avro::GenericDatum> d) : ec(ec_), schema(s), datum(d) {}
      int32_t                               ec;
      boost::shared_ptr<avro::ValidSchema>  schema;
      boost::shared_ptr<avro::GenericDatum> datum;
    };

    typedef boost::function <void(decode_result result)> decode_callback;
    typedef boost::function <void(int32_t ec)> encode_callback;
    typedef std::pair<int32_t, boost::shared_ptr<avro::ValidSchema>> get_schema_result;
    typedef boost::function <void(get_schema_result)> get_schema_callback;
    typedef std::pair<int32_t, int32_t> put_schema_result;
    typedef boost::function <void(put_schema_result)> put_callback;

    codec(confluent::registry&);

    void                 put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema>, put_callback);
    put_schema_result    put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema>);

    void                 get_schema(int32_t id, get_schema_callback);
    get_schema_result    get_schema(int32_t id);

    void    encode_nonblock(int32_t id, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream& dst);
    int32_t encode_nonblock(boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream& dst);
    void    encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst, encode_callback);
    int32_t encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst);


    template<typename T>
    void encode_nonblock(int32_t schema_id, const T& value, avro::OutputStream& dst) {
#ifdef _DEBUG
      //mutex..
      std::map<int32_t, boost::shared_ptr<avro::ValidSchema>> ::const_iterator item = _id2schema.find(schema_id);
      assert(item != _id2schema.end());
#endif
      encode_magic(dst);
      encode_schema_id(schema_id, dst);
      avro::EncoderPtr e = avro::binaryEncoder();
      e->init(dst);
      avro::encode(*e, value);
      e->flush(); // push back unused characters to the output stream again - otherwise content_length will be a multiple of 4096
    }

    template<typename T>
    std::auto_ptr<avro::OutputStream> encode_nonblock(int32_t schema_id, const T& value) {
      auto ostr = avro::memoryOutputStream();
      encode_nonblock(schema_id, value, *ostr);
      return ostr;
    }

    // change to error code bad magic, wrong schema etc
    template<typename T>
    static bool decode_static(avro::InputStream* src, int32_t id, T& dst) {
      avro::DecoderPtr e = avro::binaryDecoder();
      e->init(*src);

      if(decode_magic(src) != CONFLUENT_MAGIC_BYTE)
        return false;

      int32_t schema_id = decode_schema_id(src);

      if(id != schema_id) {
        return false;
      }

      try {
        avro::decode(*e, dst);
      } catch(...) {
        return false;
      }
      return true;
    }

    decode_result decode_datum_nonblock(avro::InputStream* src);
    void		  decode_datum(avro::InputStream* src, decode_callback);
    decode_result decode_datum(avro::InputStream* src);

    template<typename T>
    inline static bool decode_static(const uint8_t* src, size_t len, int32_t id, T& dst) {
      std::auto_ptr<avro::InputStream> stream = avro::memoryInputStream(src, len);
      return decode_static(&*stream, id, dst);
    }

    inline decode_result decode_datum_nonblock(const uint8_t* src, size_t len) {
      std::auto_ptr<avro::InputStream> stream = avro::memoryInputStream(src, len);
      return decode_datum_nonblock(&*stream);
    }

    inline void decode_datum(const uint8_t* src, size_t len, decode_callback cb) {
      std::auto_ptr<avro::InputStream> stream = avro::memoryInputStream(src, len);
      decode_datum(&*stream, cb);
    }

    inline decode_result decode_datum(const uint8_t* src, size_t len) {
      std::auto_ptr<avro::InputStream> stream = avro::memoryInputStream(src, len);
      return decode_datum(&*stream);
    }

  private:
    static void    encode_magic(avro::OutputStream& dst);
    static uint8_t decode_magic(avro::InputStream* src);

    // this is BE written as 4 bytes
    static void    encode_schema_id(int32_t schema_id, avro::OutputStream& dst);
    static int32_t decode_schema_id(avro::InputStream* src);

    confluent::registry&                                    _registry;
    std::map<boost::shared_ptr<avro::ValidSchema>, int32_t> _schema2id;
    std::map<int32_t, boost::shared_ptr<avro::ValidSchema>> _id2schema;
  };

};