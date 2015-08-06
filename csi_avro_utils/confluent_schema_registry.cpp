#include <future>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/make_shared.hpp>
#include <avro/Compiler.hh>
#include <csi_http/encoding/avro_json_spirit_encoding.h>
#include "csi_avro_utils/utils.h"
#include "confluent_schema_registry.h"

namespace confluent
{
    struct register_new_schema_req
    {
        register_new_schema_req(boost::shared_ptr<avro::ValidSchema> schema) : _schema(schema) {}
        boost::shared_ptr<avro::ValidSchema> _schema;
    };

    void json_encode(const register_new_schema_req& req, json_spirit::Object& obj)
    {
        std::stringstream ss;
        req._schema->toJson(ss);
        std::string s = ss.str();
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());  // c version does not use locale... 
        obj.push_back(json_spirit::Pair("schema", s));
    }

    struct register_new_schema_resp
    {
        register_new_schema_resp() : id(-1) {}
        int32_t id;
    };

    bool json_decode(const json_spirit::Object& obj, register_new_schema_resp& resp)
    {
        resp.id = -1;
        try
        {
          const json_spirit::Pair& item = obj[0];
          if (item.name_ == "id")
              resp.id = item.value_.get_int();
        }
        catch (std::exception& e)
        {
            BOOST_LOG_TRIVIAL(error) << "json_decode(json_spirit::Object, register_new_schema_resp) exception " << e.what();
            return false;
        }
        return true;
    }


    struct get_schema_by_id_resp
    {
        get_schema_by_id_resp() {}
        boost::shared_ptr<avro::ValidSchema> _schema;
    };


    bool json_decode(const json_spirit::Object& obj, get_schema_by_id_resp& resp)
    {
        try
        {
            const json_spirit::Pair& item = obj[0];
            if (item.name_ == "schema")
            {
                auto s = item.value_.get_str();
                resp._schema = boost::make_shared<avro::ValidSchema>(avro::compileJsonSchemaFromString(s));
            }
        }
        catch (std::exception& e)
        {
            BOOST_LOG_TRIVIAL(error) << "json_decode(json_spirit::Object, get_schema_by_id_resp) exception " << e.what();
            return false;
        }
        return true;
    }



    template<class Request>
    std::shared_ptr<csi::http_client::call_context> create_json_spirit_request(csi::http::method_t method, const std::string& uri, const Request& request, const std::vector<std::string>& headers, const std::chrono::milliseconds& timeout)
    {
        std::shared_ptr<csi::http_client::call_context> p(new csi::http_client::call_context(method, uri, headers, timeout));
        csi::json_spirit_encode(request, p->tx_content());
        std::cerr << to_string(p->tx_content()) << std::endl;
        return p;
    }

	registry::registry(boost::asio::io_service& ios, std::string address) :
		_ios(ios),
        _http(ios),
        _address(address)
    {
    }

    void registry::put_schema(std::string schema_name, boost::shared_ptr<avro::ValidSchema> schema, put_callback cb)
    {
        // http://localhost:8081/subjects/schema_name/versions
        std::string uri = "http://" + _address + "/subjects/" + schema_name + "/versions";
        _http.perform_async(
            create_json_spirit_request(
            csi::http::POST, 
            uri, 
            register_new_schema_req(schema),
            { "Content-Type: application/vnd.schemaregistry.v1+json" },
            std::chrono::milliseconds(1000)),
            [cb](csi::http_client::call_context::handle state)
        {
            int32_t result=0;
            if (state->http_result() >= 200 && state->http_result() < 300)
            {
                BOOST_LOG_TRIVIAL(debug) << "confluent::registry::put on_complete data: " << state->uri() << " got " << state->rx_content_length() << " bytes" << std::endl;
                register_new_schema_resp resp;
                if (csi::json_spirit_decode(state->rx_content(), resp))
                {
                    std::cerr << "id:" << resp.id << std::endl;
                    result = resp.id;
                    cb(state, resp.id);
                    return;
                }
                else
                {
                    BOOST_LOG_TRIVIAL(error) << "confluent::registry::put return value unexpected: " << to_string(state->rx_content());
                    cb(state, -1); // fix a bad state here!!!
                    return;
                }
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "confluent::registry::put on_complete data: " << state->uri() << " HTTPRES = " << state->http_result();
            }
            cb(state, -1);
        });
    }
    
    void registry::get_schema_by_id(int32_t id, get_callback cb)
    {
        //we should check that we don't have it in cache and post reply directy in trhat case. TBD
        
        std::string uri = "http://" + _address + "/schemas/ids/" + std::to_string(id);
        
        _http.perform_async(
            csi::create_http_request(csi::http::GET, uri, { "Content-Type: application/vnd.schemaregistry.v1+json" },
            std::chrono::milliseconds(1000)),
            [this, id, cb](csi::http_client::call_context::handle state)
        {
            if (state->http_result() >= 200 && state->http_result() < 300)
            {
                BOOST_LOG_TRIVIAL(debug) << "confluent::registry get_schema_by_id: " << state->uri() << " -> " << to_string(state->rx_content());
                get_schema_by_id_resp resp;
                if (csi::json_spirit_decode(state->rx_content(), resp))
                {
                    //mutex??
                    std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _registry.find(id);
                    if (item != _registry.end())
                    {
                        cb(state, item->second); // return the one we already have - kill the new one...
                        return;
                    }
                    std::pair<int32_t, boost::shared_ptr<avro::ValidSchema>> x(id, resp._schema);
                    _registry.insert(x);
                    cb(state, resp._schema);
                    return;
                }
                else
                {
                    BOOST_LOG_TRIVIAL(error) << "confluent::registry::get_schema_by_id return value unexpected: " << to_string(state->rx_content());
                    cb(state, NULL); // fix a bad state here!!!
                    return;
                }
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "confluent::registry get_schema_by_id uri: " << state->uri() << " HTTPRES = " << state->http_result();
            }
            cb(state, NULL);
        });
    }

     boost::shared_ptr<avro::ValidSchema> registry::get_schema_by_id(int32_t id)
    {
         //lookup in cache first
         //mutex???
         std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _registry.find(id);
         if (item != _registry.end())
             return item->second;

         //std::pair<std::shared_ptr<csi::http_client::call_context>, boost::shared_ptr<avro::ValidSchema>> 
         std::promise<std::pair<std::shared_ptr<csi::http_client::call_context>, boost::shared_ptr<avro::ValidSchema>>> p;
         std::future<std::pair<std::shared_ptr<csi::http_client::call_context>, boost::shared_ptr<avro::ValidSchema>>>  f = p.get_future();
         get_schema_by_id(id, [&p](std::shared_ptr<csi::http_client::call_context> call_context, boost::shared_ptr<avro::ValidSchema> schema)
         {
             std::pair<std::shared_ptr<csi::http_client::call_context>, boost::shared_ptr<avro::ValidSchema>> res(call_context, schema);
             p.set_value(res);
         });
         f.wait();
         std::pair<std::shared_ptr<csi::http_client::call_context>, boost::shared_ptr<avro::ValidSchema>> res = f.get();
         int32_t http_res = res.first->http_result();
         if (http_res >= 200 && http_res < 300)
         {
             //add to in cache first
             return res.second;
         }
         //exception???
         return NULL;
    }


	 //int32_t registry::get_cached_schema(boost::shared_ptr<avro::ValidSchema> p)
	 //{
		// //lookup in cache first
		// //mutex???
		// std::map<int32_t, boost::shared_ptr<avro::ValidSchema>>::const_iterator item = _registry.find(id);
		// if (item != _registry.end())
		//	 return item->second;

	 //}

}; // namespace