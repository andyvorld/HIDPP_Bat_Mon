#pragma once

#include <nlohmann/json.hpp>

namespace LGSTrayHID {
namespace WebsocketServer {
	void init_and_serve(int port);
	void notify_subscriptions(std::string path, nlohmann::json payload_json);
}
}