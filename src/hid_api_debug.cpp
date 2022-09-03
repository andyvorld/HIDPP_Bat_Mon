#include "hid_api_debug.hpp"
#ifndef _DEBUG
namespace hid_debug {
	int hid_write(hid_device* dev, unsigned char* data, size_t length) {
		return ::hid_write(dev, data, length);
	}

	int hid_read(hid_device* dev, unsigned char* data, size_t length) {
		return ::hid_read(dev, data, length);
	}

	int hid_read_timeout(hid_device* dev, unsigned char* data, size_t length, int milliseconds) {
		return ::hid_read_timeout(dev, data, length, milliseconds);
	}
}
#else
#include <mutex>
#include <iostream>
#include <string>

namespace hid_debug {
	static std::mutex print_mutex;

	static inline void print_data(char prefix, unsigned char* data, size_t length) {
		std::lock_guard lock(print_mutex);

		printf("%c: ", prefix);
		for (size_t i = 0; i < length; i++)
		{
			printf("0x%02X ", data[i]);
		}
		printf("\n");
	}

	int hid_write(hid_device* dev, unsigned char* data, size_t length) {
		print_data('W', data, length);
		return ::hid_write(dev, data, length);
	}

	int hid_read(hid_device* dev, unsigned char* data, size_t length) {
		int ret = ::hid_read(dev, data, length);
		print_data('R', data, length);
		return ret;
	}

	int hid_read_timeout(hid_device* dev, unsigned char* data, size_t length, int milliseconds) {
		int ret = ::hid_read_timeout(dev, data, length, milliseconds);
		print_data('R', data, length);
		return ret;
	}
}
#endif