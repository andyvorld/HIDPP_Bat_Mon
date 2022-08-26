#include <mutex>
#include "HIDDevice.hpp"
#include "HIDPPMsg.hpp"
#include <iostream>

namespace LGSTrayHID {
	std::mutex m;
	constexpr int HID_TIMEOUT = 100;

	HIDDevice::HIDDevice(std::string container_name) : container_name(container_name) {
	}

	HIDDevice::~HIDDevice() {
		cancellationToken = true;

		//hid_set_nonblocking(this->_short_dev.get(), 1);
		//hid_set_nonblocking(this->_long_dev.get(), 1);

		if (this->_short_reader) {
			_short_reader->join();
		}

		if (this->_long_reader) {
			_long_reader->join();
		}
	}

	void HIDDevice::_check_if_ready() {
		if (_short_reader && _long_reader) {
			constexpr size_t buf_size = 7;
			{
				uint8_t buf[buf_size] = { 0x10, 0xFF, 0x80, 0x00, 0x00, 0x01, 0x00 };
				hid_write(this->_short_dev.get(), buf, buf_size);
			}
			{
				uint8_t buf[buf_size] = { 0x10, 0xFF, 0x80, 0x02, 0x02, 0x00, 0x00 };
				hid_write(this->_short_dev.get(), buf, buf_size);
			}
		}

	}

	void HIDDevice::assign_short(std::shared_ptr<hid_device> short_dev) {
		this->_short_dev = short_dev;

		this->_short_reader = std::unique_ptr<std::thread>(new std::thread([this]() {
			while (true) {
				if (cancellationToken) {
					return;
				}

				HIDPPMsg msg(HIDPP_SHORT);
				int ret = hid_read_timeout(_short_dev.get(), msg.data(), HIDPP_SHORT_SIZE, HID_TIMEOUT);
				if (ret < 0) {
					break;
				}
				if (ret == 0) {
					continue;
				}

				m.lock();
				printf("short_dev: ");
				for (size_t i = 0; i < HIDPP_SHORT_SIZE; i++)
				{
					printf("0x%02X ", msg.data()[i]);
				}
				printf("\n");
				m.unlock();

				uint8_t dev_idx = msg.get_device_idx();

				//if (dev_idx == 0xFF) {
				//	continue;
				//}

				auto target_dev = this->devices.find(dev_idx);

				if (target_dev == this->devices.end()) {
					this->devices[dev_idx] = LogiDevice::make_shared(dev_idx, container_name, _short_dev, _long_dev);

					uint8_t buf[HIDPP_SHORT_SIZE] = { HIDPP_SHORT, dev_idx, 0x00, 0x10, 0x00, 0x00, 0x00 };
					hid_write(this->_short_dev.get(), buf, HIDPP_SHORT_SIZE);
					continue;
				}

				HIDPPMsg_10 *msg_10 = reinterpret_cast<HIDPPMsg_10*>(&msg);
				
				// On Connection notification
				if (msg_10->get_sub_id() == 0x41) {
					uint8_t buf[HIDPP_SHORT_SIZE] = { HIDPP_SHORT, dev_idx, 0x00, 0x10, 0x00, 0x00, 0x00 };
					hid_write(this->_short_dev.get(), buf, HIDPP_SHORT_SIZE);
				}
				else {
					target_dev->second->invoke_response(std::move(msg.move_ptr()));
				}
			}
		}));

		_check_if_ready();
	}

	void HIDDevice::assign_long(std::shared_ptr<hid_device> long_dev) {
		this->_long_dev = long_dev;

		this->_long_reader = std::unique_ptr<std::thread>(new std::thread([this]() {
			while (true) {
				if (cancellationToken) {
					return;
				}

				HIDPPMsg msg(HIDPP_LONG);

				int ret = hid_read_timeout(_long_dev.get(), msg.data(), HIDPP_LONG_SIZE, HID_TIMEOUT);
				if (ret < 0) {
					break;
				}
				if (ret == 0) {
					continue;
				}

				m.lock();
				printf("long_dev: ");
				for (size_t i = 0; i < HIDPP_LONG_SIZE; i++)
				{
					printf("0x%02X ", msg.data()[i]);
				}
				printf("\n");
				m.unlock();

				uint8_t dev_idx = msg.get_device_idx();

				//if (dev_idx == 0xFF) {
				//	continue;
				//}

				auto target_dev = this->devices.find(dev_idx);

				if (target_dev == this->devices.end()) {
					// STUB ignore device messages
					continue;
				}

				HIDPPMsg_20 &msg20 = reinterpret_cast<HIDPPMsg_20&>(msg);

				if (msg20.get_feature_index() >= 0x40) {
					// STUB ignore HIDPP 1.0 long messages
					continue;
				}

				if ((msg20.get_sw_id() != SW_ID) && (msg20.get_sw_id() != 0x00)) {
					// STUB ignore HIDPP 2.0 long message not tagged with SW_ID
					continue;
				}

				target_dev->second->invoke_response(msg.move_ptr());
			}
		}));

		_check_if_ready();
	}
}