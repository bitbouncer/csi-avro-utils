#include <future>
#include "confluent_codec.h"
#include <boost/make_shared.hpp>
#include <boost/endian/arithmetic.hpp>

namespace confluent {
  static bool write_raw(avro::OutputStream& dst, uint8_t* src, size_t len) {
    size_t remaining = len;
    const uint8_t* cursor = src;
    while(remaining) {
      uint8_t* buf = NULL;
      size_t capacity = 0;
      if(!dst.next(&buf, &capacity))
        return false;
      size_t bytes2write = std::min<size_t>(remaining, capacity);
      memcpy(buf, cursor, bytes2write);
      remaining -= bytes2write;
      size_t not_used = capacity - bytes2write;
      if(not_used)
        dst.backup(not_used);
    }
    return true;
  }

  static bool read_raw(avro::InputStream* src, uint8_t* dst, size_t len) {
    size_t remaining = len;
    uint8_t* cursor = dst;
    while(remaining) {
      const uint8_t* buf = NULL;
      size_t capacity = 0;
      if(!src->next(&buf, &capacity))
        return false;
      size_t bytes2read = std::min<size_t>(remaining, capacity);
      memcpy(cursor, buf, bytes2read);
      remaining -= bytes2read;
      size_t not_used = capacity - bytes2read;
      if(not_used)
        src->backup(not_used);
    }
    return true;
  }

  codec::codec(confluent::registry& r) : _registry(r) {}

  void codec::encode_magic(avro::OutputStream& dst) {
    uint8_t byte = CONFLUENT_MAGIC_BYTE;
    write_raw(dst, &byte, 1);
  }

  uint8_t codec::decode_magic(avro::InputStream* src) {
    uint8_t magic;
    if(!read_raw(src, (uint8_t*) &magic, 1))
      return 0xFF;
    return magic;
  }

  void codec::encode_schema_id(int32_t id, avro::OutputStream& dst) {
    int32_t be_id = boost::endian::native_to_big<int32_t>(id);
    write_raw(dst, (uint8_t*) &be_id, 4);
  }

  int32_t codec::decode_schema_id(avro::InputStream* src) {
    int32_t be;
    if(!read_raw(src, (uint8_t*) &be, 4))
      return -1;
    return boost::endian::big_to_native<int32_t>(be);
  }

  void codec::put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, put_callback cb) {
    _registry.put_schema(name, schema, [this, schema, cb](std::shared_ptr<csi::http_client::call_context> call_context, int32_t id) {
      if(call_context->ok()) {
        //mutex..
        _schema2id[schema] = id;
        _id2schema[id] = schema;
        cb(codec::put_schema_result(SUCCESS, id));
        return;
      } else {
        cb(codec::put_schema_result(NO_CONNECTION, 0));
        return;
      }
    });
  }

  codec::put_schema_result codec::put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema) {
    std::promise<codec::put_schema_result> p;
    std::future<codec::put_schema_result>  f = p.get_future();
    put_schema(name, schema, [&p](codec::put_schema_result result) {
      p.set_value(result);
    });
    f.wait();
    return f.get();
  }

  void codec::get_schema(int32_t id, get_schema_callback cb) {
    //mutex..
    std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _id2schema.find(id);
    if(item == _id2schema.end()) {
      _registry.get_schema_by_id(id, [this, id, cb](std::shared_ptr<csi::http_client::call_context> call_context, boost::shared_ptr<avro::ValidSchema> schema) {
        if(call_context->ok()) {
          //mutex..
          _schema2id[schema] = id;
          _id2schema[id] = schema;
          cb(codec::get_schema_result(SUCCESS, schema));
          return;
        } else {
          cb(codec::get_schema_result(NO_CONNECTION, NULL));
          return;
        }
      });
    } else {
      _registry.ios().post([cb, item] { cb(codec::get_schema_result(SUCCESS, item->second));  });
    }
  }

  codec::get_schema_result codec::get_schema(int32_t id) {
    std::promise<codec::get_schema_result> p;
    std::future<codec::get_schema_result>  f = p.get_future();
    get_schema(id, [&p](codec::get_schema_result res) {
      p.set_value(res);
    });
    f.wait();
    return f.get();
  }

  void codec::encode_nonblock(int32_t id, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream& dst) {
#ifdef _DEBUG
    //mutex..
    std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _id2schema.find(id);
    assert(item != _id2schema.end());
#endif

    encode_magic(dst);
    encode_schema_id(id, dst);
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init(dst);

    avro::encode(*e, *src);
    // push back unused characters to the output stream again... really strange... 			
    // otherwise content_length will be a multiple of 4096
    e->flush();
  }

  int32_t codec::encode_nonblock(boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream& dst) {
    //mutex..
    std::map<boost::shared_ptr<avro::ValidSchema>, int32_t>::const_iterator item = _schema2id.find(schema);
    if(item == _schema2id.end())
      return WOULD_BLOCK;

    encode_magic(dst);
    encode_schema_id(item->second, dst);
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init(dst);
    //avro::encode(*e, item->second);
    avro::encode(*e, *src);
    // push back unused characters to the output stream again... really strange... 			
    // otherwise content_length will be a multiple of 4096
    e->flush();
    return SUCCESS;
  }

  void codec::encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst, encode_callback cb) {
    put_schema(name, schema, [this, schema, src, dst, cb](codec::put_schema_result result) {
      if(result.first) {
        cb(result.first);
        return;
      }

      if(result.second > 0) {
        encode_magic(*dst);
        encode_schema_id(result.second, *dst);
        avro::EncoderPtr e = avro::binaryEncoder();
        e->init(*dst);
        avro::encode(*e, *src);
        // push back unused characters to the output stream again... really strange... 			
        // otherwise content_length will be a multiple of 4096
        e->flush();
        cb(SUCCESS);
        return;
      }
      cb(INTERNAL_SERVER_ERROR);
    });
  }

  int32_t codec::encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst) {
    std::promise<int32_t> p;
    std::future<int32_t>  f = p.get_future();
    encode(name, schema, src, dst, [&p](int32_t ec) {
      p.set_value(ec);
    });
    f.wait();
    return f.get();
  }

  codec::decode_result codec::decode_datum_nonblock(avro::InputStream* stream) {
    avro::DecoderPtr decoder = avro::binaryDecoder();
    decoder->init(*stream);

    if(decode_magic(stream) != CONFLUENT_MAGIC_BYTE)
      return codec::decode_result(NOT_AVRO, NULL, NULL);

    int32_t schema_id = decode_schema_id(stream);
    //mutex..
    std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _id2schema.find(schema_id);
    if(item == _id2schema.end())
      return codec::decode_result(WOULD_BLOCK, NULL, NULL);

    if(item->second == NULL)
      return codec::decode_result(INTERNAL_SERVER_ERROR, NULL, NULL);

    boost::shared_ptr<avro::GenericDatum> p = boost::make_shared<avro::GenericDatum>(*item->second);
    avro::decode(*decoder, *p);
    return codec::decode_result(SUCCESS, item->second, p);
  }

  void codec::decode_datum(avro::InputStream* stream, decode_callback cb) {
    avro::DecoderPtr decoder = avro::binaryDecoder();
    decoder->init(*stream);

    if(decode_magic(stream) != CONFLUENT_MAGIC_BYTE)
      cb(codec::decode_result(NOT_AVRO, NULL, NULL));

    int32_t schema_id = decode_schema_id(stream);
    get_schema(schema_id, [decoder, cb](get_schema_result res) {
      if(res.first) // ec
      {
        cb(codec::decode_result(res.first, NULL, NULL));
        return;
      }

      if(res.second == NULL) // this should never happen
      {
        cb(codec::decode_result(INTERNAL_SERVER_ERROR, NULL, NULL));
        return;
      }

      boost::shared_ptr<avro::GenericDatum> p = boost::make_shared<avro::GenericDatum>(*res.second);
      avro::decode(*decoder, *p);
      cb(codec::decode_result(SUCCESS, res.second, p));
      return;
    });
  }

  codec::decode_result codec::decode_datum(avro::InputStream* stream) {
    std::promise<codec::decode_result> p;
    std::future<codec::decode_result>  f = p.get_future();
    decode_datum(stream, [&p](codec::decode_result res) {
      p.set_value(res);
    });
    f.wait();
    return f.get();
  }

  std::string codec::to_string(error_code_t ec) {
    switch(ec) {
    case SUCCESS: return "SUCCESS";
    case WOULD_BLOCK: return "WOULD_BLOCK";
    case NOT_FOUND: return "NOT_FOUND";
    case NO_CONNECTION: return "NO_CONNECTION";
    case INTERNAL_SERVER_ERROR: return "INTERNAL_SERVER_ERROR";
    case NOT_AVRO: return "NOT_AVRO";
    };
    return "UNKNOWN_ERROR:" + ec;
  }
};
