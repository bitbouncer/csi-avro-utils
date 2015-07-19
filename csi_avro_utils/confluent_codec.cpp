#include <future>
#include "confluent_codec.h"
#include <boost/make_shared.hpp>

namespace confluent
{
	codec::codec(confluent::registry& r) : _registry(r)
	{
	}

	void codec::put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, put_callback cb)
	{
		_registry.put_schema(name, schema, [this, schema, cb](std::shared_ptr<csi::http_client::call_context> call_context, int32_t id)
		{
			if (call_context->ok())
			{
				//mutex..
				_schema2id[schema] = id;
				_id2schema[id]     = schema;
				cb(SUCCESS, id);
				return;
			}
			else
			{
				cb(NO_CONNECTION, 0);
				return;
			}
		});
	}

	int32_t codec::put_schema(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema)
	{
		std::promise<int32_t> p;
		std::future<int32_t>  f = p.get_future();
		put_schema(name, schema, [&p](int32_t ec, int32_t id)
		{
			p.set_value(ec);
		});
		f.wait();
		return f.get();
	}


	void codec::get_schema(int32_t id, get_schema_callback cb)
	{
		//mutex..
		std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _id2schema.find(id);
		if (item == _id2schema.end())
		{
			_registry.get_schema_by_id(id, [this, id, cb](std::shared_ptr<csi::http_client::call_context> call_context, boost::shared_ptr<avro::ValidSchema> schema)
			{
				if (call_context->ok())
				{
					//mutex..
					_schema2id[schema] = id;
					_id2schema[id] = schema;
					cb(codec::get_schema_result(SUCCESS, schema));
					return;
				}
				else
				{
					cb(codec::get_schema_result(NO_CONNECTION, NULL));
					return;
				}
			});
		}
		else
		{
			_registry.ios().post([cb, item]{ cb(codec::get_schema_result(SUCCESS, item->second));  });
		}
	}

	codec::get_schema_result codec::get_schema(int32_t id)
	{
		std::promise<codec::get_schema_result> p;
		std::future<codec::get_schema_result>  f = p.get_future();
		get_schema(id, [&p](codec::get_schema_result res)
		{
			p.set_value(res);
		});
		f.wait();
		return f.get();
	}


	int32_t codec::encode_nonblock(boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream& dst)
	{
		//mutex..
		std::map<boost::shared_ptr<avro::ValidSchema>, int32_t>::const_iterator item = _schema2id.find(schema);
		if (item == _schema2id.end())
			return WOULD_BLOCK;

		avro::EncoderPtr e = avro::binaryEncoder();
		e->init(dst);
		avro::encode(*e, item->second);
		avro::encode(*e, *src);
		// push back unused characters to the output stream again... really strange... 			
		// otherwise content_length will be a multiple of 4096
		e->flush();
		return SUCCESS;
	}

	void codec::encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst, encode_callback cb)
	{
		put_schema(name, schema, [this, schema, src, dst, cb](int32_t ec, int32_t id)
		{
			if (ec)
			{
				cb(ec);
				return;
			}

			if (id > 0)
			{
				avro::EncoderPtr e = avro::binaryEncoder();
				e->init(*dst);
				avro::encode(*e, id);
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

	int32_t codec::encode(const std::string& name, boost::shared_ptr<avro::ValidSchema> schema, boost::shared_ptr<avro::GenericDatum> src, avro::OutputStream* dst)
	{
		std::promise<int32_t> p;
		std::future<int32_t>  f = p.get_future();
		encode(name, schema, src, dst, [&p](int32_t ec)
		{
			p.set_value(ec);
		});
		f.wait();
		return f.get();
	}

	codec::decode_result codec::decode_datum_nonblock(avro::InputStream* stream)
	{
		avro::DecoderPtr decoder = avro::binaryDecoder();
		decoder->init(*stream);
		int32_t schema_id = 0;
		avro::decode(*decoder, schema_id);

		//mutex..
		std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _id2schema.find(schema_id);
		if (item == _id2schema.end())
			return codec::decode_result(WOULD_BLOCK, NULL, NULL);
		
		if (item->second == NULL)
			return codec::decode_result(INTERNAL_SERVER_ERROR, NULL, NULL);

		boost::shared_ptr<avro::GenericDatum> p = boost::make_shared<avro::GenericDatum>(*item->second);
		avro::decode(*decoder, *p);
		return codec::decode_result(SUCCESS, item->second, p);
	}

	void codec::decode_datum(avro::InputStream* stream, decode_callback cb)
	{
		avro::DecoderPtr decoder = avro::binaryDecoder();
		decoder->init(*stream);
		int32_t schema_id = 0;
		avro::decode(*decoder, schema_id);

		get_schema(schema_id, [decoder, cb](get_schema_result res)
		{
			if (res.first) // ec
			{
				cb(codec::decode_result(res.first, NULL, NULL));
				return;
			}

			if (res.second == NULL) // this should never happen
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

	codec::decode_result codec::decode_datum(avro::InputStream* stream)
	{
		std::promise<codec::decode_result> p;
		std::future<codec::decode_result>  f = p.get_future();
		decode_datum(stream, [&p](codec::decode_result res)
		{
			p.set_value(res);
		});
		f.wait();
		return f.get();
	}
};