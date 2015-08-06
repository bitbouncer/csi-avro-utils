#include <avro/Specific.hh>
#include <avro/Encoder.hh>
#include <avro/Decoder.hh>
//#include "utils.h"

#pragma once

namespace csi
{
 	// encodes id first in 4 bytes ( linkedin/confluent codec style )
	template<class T>
	void avro_binary_encode_with_schema_id(int32_t id, const T& src, avro::OutputStream& dst)
	{
		avro::EncoderPtr e = avro::binaryEncoder();
		e->init(dst);
		avro::encode(*e, id);
		avro::encode(*e, src);
		// push back unused characters to the output stream again... really strange... 			
		// otherwise content_length will be a multiple of 4096
		e->flush();
	}

	template<class T>
	bool avro_binary_decode_with_schema_id(avro::InputStream& is, int32_t id, T& dst)
	{
		int32_t schema_id;
		avro::DecoderPtr e = avro::binaryDecoder();
		e->init(is);
		avro::decode(*e, schema_id);
		if (id != schema_id)
		{
			return false;
		}

		try
		{
			avro::decode(*e, dst);
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
}; // csi

