#include "MapWrapper.hpp"

#include <sstream>

#include <mutex>
#include <map>

namespace LGSTrayHID {
	namespace MapWrapper {
		std::mutex logiDevice_map_mutex;
		std::map<KeyType, std::weak_ptr<LogiDevice>> logiDevice_map;

		void add_LogiDevice(KeyType key, const std::shared_ptr<LogiDevice>& logiDevice) {
			const std::lock_guard<std::mutex> lock(logiDevice_map_mutex);
			logiDevice_map[key] = logiDevice;
		}

		void clear_map() {
			const std::lock_guard<std::mutex> lock(logiDevice_map_mutex);
			logiDevice_map.clear();
		}
	}
}