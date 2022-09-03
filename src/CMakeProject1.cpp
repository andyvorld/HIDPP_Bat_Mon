// CMakeProject1.cpp : Defines the entry point for the application.
//
#include "CMakeProject1.h"

#include <type_traits>
#include <chrono>
#include <thread>
#include <map>
#include <functional>

#include <hidapi.h>

#include <cassert>

#ifdef _WIN32
#include <hidapi_winapi.h>
#endif

#include "HIDDevice.hpp"
#include "HIDPPMsg.hpp"

#include "MapWrapper.hpp"

namespace {
	enum USAGE_PAGE : uint8_t {
		SHORT = 0x01,
		LONG = 0x02,
		VERY_LONG = 0x04
	};
}

std::string GUID_to_string(const GUID& guid) {
	std::ostringstream os;

	os << std::hex << guid.Data1;
	os << std::hex << guid.Data2;
	os << std::hex << guid.Data3;

	for (size_t i = 0; i < 8; i++) {
		os << std::hex << static_cast<int>(guid.Data4[i]);
	}

	return os.str();
}

struct GUIDComparer
{
	bool operator()(const GUID& Left, const GUID& Right) const
	{
		// comparison logic goes here
		return memcmp(&Left, &Right, sizeof(Right)) < 0;
	}
};

using namespace std::placeholders;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
	LGSTrayHID::LogiDevice::Register_battery_update_cb([](const LGSTrayHID::LogiDevice& dev) {
	});

	LGSTrayHID::LogiDevice::Register_device_ready_cb([](const LGSTrayHID::LogiDevice& dev) {
	});

	hid_init();

	auto devs = std::unique_ptr<hid_device_info, decltype(&hid_free_enumeration)>(hid_enumerate(0x046d, 0x00), &hid_free_enumeration);
	auto cur_dev = devs.get();

	std::map<GUID, std::unique_ptr<LGSTrayHID::HIDDevice>, GUIDComparer> hid_device_map;

	while (cur_dev) {
		if ((cur_dev->usage_page & 0xFF00) == 0xFF00) {
			if ((cur_dev->usage == USAGE_PAGE::SHORT) || (cur_dev->usage == USAGE_PAGE::LONG)) {
				auto c_hid_device = std::shared_ptr<hid_device>(hid_open_path(cur_dev->path), &hid_close);
				GUID _guid;
				hid_winapi_get_container_id(c_hid_device.get(), &_guid);

				bool found = (hid_device_map.find(_guid) != hid_device_map.end());

				if (!found) {
					hid_device_map[_guid] = std::unique_ptr<LGSTrayHID::HIDDevice>(new LGSTrayHID::HIDDevice(GUID_to_string(_guid)));
				}

				if (cur_dev->usage == USAGE_PAGE::SHORT) {
					hid_device_map[_guid]->assign_short(c_hid_device);
				}
				else if (cur_dev->usage == USAGE_PAGE::LONG) {
					hid_device_map[_guid]->assign_long(c_hid_device);
				}
			}
		}

		cur_dev = cur_dev->next;
	}

	std::this_thread::sleep_for(10s);

	return 0;
}
