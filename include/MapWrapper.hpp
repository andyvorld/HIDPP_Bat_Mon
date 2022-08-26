#pragma once

#include <memory>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "LogiDevice.hpp"

namespace LGSTrayHID {
	namespace MapWrapper {
		typedef std::string KeyType;
		void add_LogiDevice(KeyType key, const std::shared_ptr<LogiDevice>& logiDevice);

		nlohmann::json to_json();
		nlohmann::json get_battery_summary(KeyType key);

		void clear_map();
	}
}