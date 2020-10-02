#pragma once
#include <winrt\base.h>
using namespace winrt;

#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;

namespace helpers {
	bool TryParseSigDefinedUuid(guid const& uuid, uint16_t& shortId);

	hstring GetServiceName(GattDeviceService const& service);

	hstring GetCharacteristicName(GattCharacteristic const& characteristic);
}
