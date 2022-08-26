#include "WebsocketServer.hpp"

#include <nlohmann/json.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "MapWrapper.hpp"

using namespace std::placeholders;


namespace LGSTrayHID {
namespace WebsocketServer {
	typedef websocketpp::server<websocketpp::config::asio> Server;
	typedef Server::message_ptr message_ptr;

	namespace {
		struct Subscription_map_value {
			Server* s;
			websocketpp::connection_hdl hdl;
		};
	}

	typedef std::multimap<std::string, Subscription_map_value> Subscription_map;

	Server server;
	Subscription_map subscription_map;

	nlohmann::json gen_message_json(std::string msg_id, std::string verb, std::string path, nlohmann::json payload, std::string res_code) {
		nlohmann::json ret =
		{
			{"msgId", msg_id},
			{"origin", "backend"},
			{"verb", verb},
			{"path", path},
			{"payload", payload}
		};
		ret["result"]["code"] = res_code;

		return ret;
	}

	void on_get(Server* s, websocketpp::connection_hdl hdl, nlohmann::json payload_json) {
		const std::string path = payload_json.value("path", "");

		bool temp = (s == &server);

		if (path.compare("/devices/list") == 0) {
			std::cout << "requested /devices/list" << std::endl;

			std::string ret_payload = gen_message_json("", "GET", "/devices/list", MapWrapper::to_json(), "SUCCESS").dump();

			s->send(hdl, ret_payload, websocketpp::frame::opcode::text);
		}
		else if (path.compare(0, 8, "/battery") == 0) {
			std::cout << "requested /battery" << std::endl;
			std::cout << "payload " << payload_json.dump() << std::endl;

			size_t delim1 = path.find('/', 1);
			size_t delim2 = path.find('/', delim1+1);

			if (delim2 < delim1) {
				return;
			}

			std::string dev_id = path.substr(delim1+1, delim2-delim1-1);
			std::string ret_payload;

			try {
				auto battery_summary = MapWrapper::get_battery_summary(dev_id);
				ret_payload = gen_message_json("", "GET", path, battery_summary, "SUCCESS").dump();
			}
			catch (std::out_of_range&) {
				ret_payload = gen_message_json("", "GET", path, {}, "NO_SUCH_PATH").dump();
			}

			s->send(hdl, ret_payload, websocketpp::frame::opcode::text);
		}
	}

	void on_subscribe(Server* s, websocketpp::connection_hdl hdl, nlohmann::json payload_json) {
		const std::string path = payload_json.value("path", "");

		if (path.empty()) {
			return;
		}

		subscription_map.emplace(path, Subscription_map_value{s, hdl});
	}

	void on_message(Server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
		//std::cout << "on_message called with hdl: " << hdl.lock().get()
		//	<< " and message: " << msg->get_payload()
		//	<< std::endl;

		//try {
		//	s->send(hdl, msg->get_payload(), msg->get_opcode());
		//}
		//catch (websocketpp::exception const& e) {
		//	std::cout << "Echo failed because: "
		//		<< "(" << e.what() << ")" << std::endl;
		//}

		nlohmann::json payload_json = nlohmann::json::parse(msg->get_payload());

		const std::string verb = payload_json.value("verb", "");

		if (verb.compare("GET") == 0) {
			on_get(s, hdl, payload_json);
		}
		else if (verb.compare("SUBSCRIBE") == 0) {
			on_subscribe(s, hdl, payload_json);
		}
	}

	void init_and_serve(int port) {
		server.init_asio();

		server.set_message_handler(std::bind(&on_message, &server, _1, _2));

		server.listen(port);
		server.start_accept();

		server.run();
	}

	void notify_subscriptions(std::string path, nlohmann::json payload_json) {
		auto subscription_entries = subscription_map.equal_range(path);
		auto entry_length = std::distance(subscription_entries.first, subscription_entries.second);

		//if (entry_length == 0) {
		//	return;
		//}

		nlohmann::json output_payload = gen_message_json("", "BROADCAST", path, payload_json, "SUCCESS");

		auto temp = output_payload.dump();

		for (Subscription_map::iterator entry_iter = subscription_entries.first; entry_iter != subscription_entries.second;) {
			auto& entry = entry_iter->second;
			if (entry.hdl.expired()) {
				entry_iter = subscription_map.erase(entry_iter);
				continue;
			}

			entry.s->send(entry.hdl, output_payload.dump(), websocketpp::frame::opcode::text);

			++entry_iter;
		}
	}
}
}