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
#include "WebsocketServer.hpp"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <nlohmann/json.hpp>

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

typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr message_ptr;

void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
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
}

using namespace std::placeholders;

int main() {
	LGSTrayHID::LogiDevice::Register_battery_update_cb([](const LGSTrayHID::LogiDevice& dev) {
		std::cout << dev.battery_summary().dump() << std::endl;
		std::cout << LGSTrayHID::MapWrapper::to_json().dump() << std::endl;

		LGSTrayHID::WebsocketServer::notify_subscriptions("/battery/state/changed", dev.battery_summary());
	});

	LGSTrayHID::LogiDevice::Register_device_ready_cb([](const LGSTrayHID::LogiDevice& dev) {
		LGSTrayHID::WebsocketServer::notify_subscriptions("/devices/state/changed", LGSTrayHID::MapWrapper::to_json());
	});

	//for (size_t ii = 0; ii < 1; ii++) {
		hid_init();

		auto devs = std::unique_ptr<hid_device_info, decltype(&hid_free_enumeration)>(hid_enumerate(0x046d, 0x00), &hid_free_enumeration);
		auto cur_dev = devs.get();

		std::string short_path;
		std::string long_path;


		std::map<GUID, std::unique_ptr<LGSTrayHID::HIDDevice>, GUIDComparer> hid_device_map;

		while (cur_dev) {
			//printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
			//printf("\n");
			//printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
			//printf("  Product:      %ls\n", cur_dev->product_string);
			//printf("  Release:      %hx\n", cur_dev->release_number);
			//printf("  Interface:    %d\n", cur_dev->interface_number);
			//printf("  Usage (page): 0x%hx (0x%hx)\n", cur_dev->usage, cur_dev->usage_page);
			//printf("\n");

			if ((cur_dev->usage_page & 0xFF00) == 0xFF00) {
				//if (cur_dev->usage == USAGE_PAGE::SHORT) {
				//	short_path.assign(cur_dev->path);
				//}
				//else if (cur_dev->usage == USAGE_PAGE::LONG) {
				//	long_path.assign(cur_dev->path);
				//}

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

		//auto short_dev = std::shared_ptr<hid_device>(hid_open_path(short_path.c_str()), &hid_close);
		//auto long_dev = std::shared_ptr<hid_device>(hid_open_path(long_path.c_str()), &hid_close);

		//{
		//	GUID tmp;
		//	hid_winapi_get_container_id(short_dev.get(), &tmp);
		//	std::cout << std::hex << tmp.Data1;
		//	std::cout << std::hex << tmp.Data2;
		//	std::cout << std::hex << tmp.Data3;
		//	for (int i = 0; i < 8; i++) {
		//		std::cout << std::hex << (int)tmp.Data4[i];
		//	}
		//}
		//std::cout << std::endl;

		//LGSTrayHID::HIDDevice HidDevice;
		//HidDevice.assign_short(short_dev);
		//HidDevice.assign_long(long_dev);
		int i = 0;
		while (true) {
			i++;
			for (const auto& pair : hid_device_map) {
				for (const auto& pair2 : pair.second->devices) {
					pair2.second->update_battery();
				}
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));

			if (i > 5) {
				break;
			}
		}
	//}

	LGSTrayHID::WebsocketServer::init_and_serve(9020);

	if (_CrtDumpMemoryLeaks()) {
		std::cout << "Mem Leak Found" << std::endl;
	}

	return 0;
}
