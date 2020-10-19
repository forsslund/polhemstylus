#include "pch.h"
#include "BLEdeviceFinder.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Storage.Streams.h"
using namespace winrt;
using namespace Windows::Foundation;

BLEdeviceFinder* BLEdeviceFinder::instance = 0;

#include <iostream>
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;
//#include "helpers.h"
//using namespace helpers;

constexpr guid stylusService			 = { 0x90ad0000, 0x662b, 0x4504, { 0xb8, 0x40, 0x0f, 0xf1, 0xdd, 0x28, 0xd8, 0x4e } };
constexpr guid stylusValueCharacteristic = { 0x90ad0001, 0x662b, 0x4504, { 0xb8, 0x40, 0x0f, 0xf1, 0xdd, 0x28, 0xd8, 0x4e } };

struct StylusData {
	bool button;
	uint16_t rotation;
};

fire_and_forget stylusValueHandler(GattCharacteristic c, GattValueChangedEventArgs const& v) {	
	wcout << "Got: " << v.CharacteristicValue().Length() << " bytes: "<< std::hex;
	
	for (size_t i = 0; i < v.CharacteristicValue().Length(); i++) {
		wcout << v.CharacteristicValue().data()[i] << " ";
	}

	StylusData s = {0};
	s.button = (0x80 & v.CharacteristicValue().data()[0]) != 0;
	s.rotation = (0x03 & v.CharacteristicValue().data()[0]) * 256 + v.CharacteristicValue().data()[1];
	wcout << "\nRotation: "<< std::dec << s.rotation << (s.button ? " Button down" : " Button up") <<  "\n\n";
	return winrt::fire_and_forget();
}

int main()
{
    init_apartment();
	char userInput[3];
	userInput[0] = '\0';

 	BLEdeviceFinder* pBleFinder = BLEdeviceFinder::getInstance();
    hstring devId = pBleFinder->PromptUserForDevice();

	if (devId!=L"") {
		try {			
			// Connect to the device by the Id
			// Creating a BluetoothLEDevice object by calling this method alone doesn't (necessarily) initiate a connection.
			Windows::Devices::Bluetooth::BluetoothLEDevice bleDev{ BluetoothLEDevice::FromIdAsync(devId).get() };

			if (bleDev == nullptr) {
				wcout << "Failed to connect to device " << devId.c_str() << ".\n";
				return 1;
			}

			// Print some device info
			wcout << endl
				<< "Name: " << bleDev.Name().c_str() << endl
				<< "Connection status:" << (bleDev.ConnectionStatus() == BluetoothConnectionStatus::Connected ? "Connected" : "Not Connected") << endl
				<< "BluetoothAddress: " << std::hex << bleDev.BluetoothAddress() << endl
				<< "DeviceAccessInformation: " << (uint16_t)bleDev.DeviceAccessInformation().CurrentStatus() << endl;

			//
			// Search device for the service we want
			//
			auto gattServices{ bleDev.GetGattServicesAsync(BluetoothCacheMode::Uncached).get() };
			for (GattDeviceService service : gattServices.Services()) {
				GattCommunicationStatus gattCommunicationStatus = gattServices.Status();
				if (gattCommunicationStatus == GattCommunicationStatus::Success) {
					if (service.Uuid() == stylusService) {						
						auto result = service.GetCharacteristicsForUuidAsync(stylusValueCharacteristic);
						
						//
						// Let the com thread have some time to communicate
						//
						size_t i = 0;
						while (!(result.Completed() || result.ErrorCode() || i < 100)) {
							_sleep(100);
						}
						
						if (result.ErrorCode()) {
							wcout << "GetCharacteristicsForUuidAsync(stylusValueCharacteristic) got error code " << result.ErrorCode() << ".\n";
						}
						else if (result.get().Status() == GattCommunicationStatus::Success){
							// Should only be one, but we get a vector, lets iterate						
							for (GattCharacteristic c : result.get().Characteristics()) {								
								wcout	<< "Found matching GattCharacteristic "<< to_hstring(c.Uuid()).c_str()<< endl								
										<< "Press enter to start subscribing to notifications, press enter again to stop:\n";
								cin.getline(userInput, 2);

								//
								// Check that characteristic is writable. Themn write to it to tell the dev that we want notifications								
								//
								if (GattCharacteristicProperties::None != (c.CharacteristicProperties() & GattCharacteristicProperties::Notify) ) {
									event_token t = c.ValueChanged(stylusValueHandler);
									c.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
									cin.getline(userInput, 2);
								}
							}							
						}
						else {
							wcout << "Com error"<< (int) result.GetResults().Status()<<"\n";
						}
					}
				}
			}			
			bleDev.Close();
		}
		catch(...){
			wcout << "\nError or device connection lost.\n";			
		}
	}
	wcout << "Enter to quit";
	cin.getline(userInput, 2);
	return 0;
}

