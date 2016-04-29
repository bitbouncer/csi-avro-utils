#include <memory>
#include <cstring>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/array.hpp>
#include <avro/Stream.hh>
#include <avro/ValidSchema.hh>
#include <avro/Generic.hh>

#pragma once

inline boost::array<uint8_t, 16> to_array(const boost::uuids::uuid& uuid) {
  boost::array<uint8_t, 16> a;
  memcpy(a.c_array(), uuid.data, 16);
  return a;
}

inline boost::uuids::uuid to_uuid(const boost::array<uint8_t, 16>& a) {
  boost::uuids::uuid uuid;
  memcpy(uuid.data, a.data(), 16);
  return uuid;
}

inline std::string to_string(const boost::array<uint8_t, 16>& a) {
  boost::uuids::uuid uuid;
  memcpy(uuid.data, a.data(), 16);
  return to_string(uuid);
}

//inefficient but useful for debugging and error handling -- remove???
/*
inline std::string to_string(std::auto_ptr<avro::InputStream> pstream)
{
std::string s;
const uint8_t* buf;
size_t len;
while (pstream->next(&buf, &len))
s.append((const char*)buf, len);
return s;
}
*/

std::string        to_string(const avro::OutputStream& os);
boost::uuids::uuid generate_hash(const avro::ValidSchema&);
std::string        to_string(const avro::ValidSchema& vs);
std::string        normalize(const avro::ValidSchema&);


// should probably go into namespace csi::avro
// not implemented for 
// AVRO_NULL (not relevant)
// AVRO_RECORD 
// AVRO_ENUM  - dont know how... 
// AVRO_ARRAY - dont know how... 
// AVRO_MAP
// AVRO_UNION
// AVRO_FIXED

template<typename T> avro::Type CPPToAvroType();
template<> inline avro::Type CPPToAvroType<std::string>() { return avro::AVRO_STRING; }
template<> inline avro::Type CPPToAvroType<std::vector<uint8_t>>() { return avro::AVRO_BYTES; }
template<> inline avro::Type CPPToAvroType<int32_t>() { return avro::AVRO_INT; }
template<> inline avro::Type CPPToAvroType<int64_t>() { return avro::AVRO_LONG; }
template<> inline avro::Type CPPToAvroType<float>() { return avro::AVRO_FLOAT; }
template<> inline avro::Type CPPToAvroType<double>() { return avro::AVRO_DOUBLE; }
template<> inline avro::Type CPPToAvroType<bool>() { return avro::AVRO_BOOL; }

// temp code - better names??
// extracts field by name - either direct member or current branch of union
// better exceptions
template<typename T> T get_field_by_name(const avro::GenericDatum& gd, const std::string& field_name) {
  if(gd.type() != avro::AVRO_RECORD)
    throw std::domain_error(std::string("expected: AVRO_RECORD, actual: ") + avro::toString(gd.type()));

  const avro::GenericRecord& record = gd.value<avro::GenericRecord>();
  if(!record.hasField(field_name))
    throw std::domain_error(std::string("no such field: ") + field_name);

  const avro::GenericDatum& field = record.field(field_name);

  if(field.type() == CPPToAvroType<T>())
    return field.value<T>();

  if(field.isUnion()) {
    const avro::GenericUnion& generic_union = field.value<avro::GenericUnion>();
    if(generic_union.datum().type() == CPPToAvroType<T>())
      return generic_union.datum().value<T>();
    else
      throw std::bad_cast();
  }
  throw std::bad_cast();
}
