#include "MapWrapper.hpp"

#include <sstream>

#include <mutex>
#include <map>

#include <nlohmann/json.hpp>

namespace LGSTrayHID {
	namespace MapWrapper {
		std::mutex logiDevice_map_mutex;
		std::map<KeyType, std::weak_ptr<LogiDevice>> logiDevice_map;

		void add_LogiDevice(KeyType key, const std::shared_ptr<LogiDevice>& logiDevice) {
			const std::lock_guard<std::mutex> lock(logiDevice_map_mutex);
			logiDevice_map[key] = logiDevice;
		}

		nlohmann::json to_json() {
			nlohmann::json json_payload;
			json_payload["deviceInfos"] = nlohmann::json::array();

			const std::lock_guard<std::mutex> lock(logiDevice_map_mutex);

			for (const auto &pair : logiDevice_map) {
				const std::shared_ptr<const LogiDevice> device = pair.second.lock();
				if (device == nullptr) {
					continue;
				}

				json_payload["deviceInfos"].insert(
					json_payload["deviceInfos"].end(),
					device->device_summary()
				);
			}

			return json_payload;
		}

		nlohmann::json get_battery_summary(KeyType key)
		{
			const std::lock_guard<std::mutex> lock(logiDevice_map_mutex);
			const std::shared_ptr<const LogiDevice> device = logiDevice_map[key].lock();
			if (device == nullptr) {
				throw std::out_of_range("Invalid device id");
			}

			return device->battery_summary();
		}

		void clear_map() {
			const std::lock_guard<std::mutex> lock(logiDevice_map_mutex);
			logiDevice_map.clear();
		}
	}
}