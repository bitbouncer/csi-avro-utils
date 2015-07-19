#include "confluent_schema_registry.h"
#pragma once

namespace confluent
{
	class codec
	{
	public:
		enum { SUCCESS = 0, WOULD_BLOCK = 1, NOT_FOUND = 2, NO_CONNECTION = 3, INTERNAL_SERVER_ERROR=4 };
		
		struct decode_result
		{
			decode_result() : ec(SUCCESS) {}
			decode_result(int32_t ec_, boost::shared_ptr<avro::ValidSchema> s, boost::shared_ptr<avro::GenericDatum> d) : ec(ec_), schema(s), datum(d) {} 
			int32_t                               ec;
			boost::shared_ptr<avro::ValidSchema>  schema;
			boost::shared_ptr<avro::GenericDatum> datum;
		};

		//typedef std::pair<int32_t, boost::shared_ptr<avro::GenericDatum>> decode_result;
		typedef boost::function <void(decode_result result)>              decode_callback;

		typedef boost::function <void(int32_t ec)> encode_callback;

		typedef std::pair<int32_t, boost::shared_ptr<avro::ValidSchema>>  get_schema_result;
		typedef boost::function <void(get_schema_result)>			      get_schema_callback;

		typedef boost::function <void(int32_t ec, int32_t id)>			  put_callback;

		codec(confluent::registry&);

		void    put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema>, put_callback);
		int32_t put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema>);

		void                 get_schema(int32_t id, get_schema_callback);
		get_schema_result    get_schema(int32_t id);

		int32_t encode_nonblock(boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream& dst);
		void    encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst, encode_callback);
		int32_t encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst);

		decode_result decode_datum_nonblock(avro::InputStream* src);
		void		  decode_datum(avro::InputStream* src, decode_callback);
		decode_result decode_datum(avro::InputStream* src);

	private:
		confluent::registry& _registry;
		std::map<boost::shared_ptr<avro::ValidSchema>, int32_t> _schema2id;
		std::map<int32_t, boost::shared_ptr<avro::ValidSchema>> _id2schema;
	};

};