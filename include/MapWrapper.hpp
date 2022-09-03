#pragma once

#include <memory>
#include <unordered_map>

#include "LogiDevice.hpp"

namespace LGSTrayHID {
	namespace MapWrapper {
		typedef std::string KeyType;
		void add_LogiDevice(KeyType key, const std::shared_ptr<LogiDevice>& logiDevice);

		void clear_map();
	}
}