#pragma once
#include <hidapi.h>

namespace hid_debug {
	int hid_write(hid_device* dev, unsigned char* data, size_t length);
	int hid_read(hid_device* dev, unsigned char* data, size_t length);
	int hid_read_timeout(hid_device* dev, unsigned char* data, size_t length, int milliseconds);
}