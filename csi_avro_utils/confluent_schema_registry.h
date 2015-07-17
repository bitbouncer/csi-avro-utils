#include <map>
#include <csi_http/client/http_client.h>
#include <avro/Generic.hh>
#include <avro/Schema.hh>

#pragma once

namespace confluent
{
    class registry
    {
    public:
        typedef boost::function <void(std::shared_ptr<csi::http_client::call_context> call_context, int32_t result)> put_callback;
        typedef boost::function <void(std::shared_ptr<csi::http_client::call_context> call_context, boost::shared_ptr<avro::ValidSchema>)> get_callback;

        registry(boost::asio::io_service& ios, std::string address); // how to give a vector??
        void put_schema(std::string name, boost::shared_ptr<avro::ValidSchema>, put_callback);
        void get_schema_by_id(int32_t id, get_callback);
        boost::shared_ptr<avro::ValidSchema> get_schema_by_id(int32_t id);
    private:
        csi::http_client                                        _http;
        std::string                                             _address;
        std::map<int32_t, boost::shared_ptr<avro::ValidSchema>> _registry;
    };
};