#include "pch.h"
#include "BLEdeviceFinder.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Storage.Streams.h"
#include "external/httplib.h"
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

bool verbose = true;

struct StylusData {
	bool button;
	uint16_t rotation;
};

httplib::Client* pCli;

fire_and_forget stylusValueHandler(GattCharacteristic c, GattValueChangedEventArgs const& v) {	
	if (verbose) {
		wcout << "Got: " << v.CharacteristicValue().Length() << " bytes: " << std::hex;
		for (size_t i = 0; i < v.CharacteristicValue().Length(); i++) {
			wcout << v.CharacteristicValue().data()[i] << " ";
		}
	}

	StylusData s = {0};
	s.button = (0x80 & v.CharacteristicValue().data()[0]) != 0;
	s.rotation = (0x03 & v.CharacteristicValue().data()[0]) * 256 + v.CharacteristicValue().data()[1];
	if (verbose) wcout << "\nRotation: "<< std::dec << s.rotation << (s.button ? " Button down" : " Button up") <<  "\n\n";
	char getRequestString[80];
	snprintf(getRequestString, 80, "/set?btn=%d&enc=%d", s.button ? 1 : 0, s.rotation);
	if(pCli!=NULL){
		if (auto res = pCli->Get(getRequestString)) {
			if(verbose){
				if (res->status == 200) {
					std::cout << res->body << std::endl;
					std::cout << "status200\n";
				}		
			}
		}
		else {
			auto err = res.error();
			std::cout << "HTTP COM ERROR\n";
		}
	}
	else {
		std::cout << "client gone\n";
	}

	return winrt::fire_and_forget();
}

int main(int argc, char* argv[])
{
	init_apartment();

	uint64_t deviceAddress = 0;
	if (argc == 2) {
		deviceAddress = stoll(argv[1], 0, 16);	// Convert adress string in hex format
	}
	if (argc != 1 && deviceAddress == 0) {
		wcout << "Usage\t" << argv[1] << "\t\t" << "\t" << "Connect to deviceId (hex)" << endl
			<< "\t" << argv[1] << " deviceId" << "\t" << "Connect to deviceId (hex)" << endl;
		return 1;
	}

	char userInput[3];
	userInput[0] = '\0';
	hstring devId;
	if (deviceAddress == 0) {
		BLEdeviceFinder* pBleFinder = BLEdeviceFinder::getInstance();
		devId = pBleFinder->PromptUserForDevice();
		cout << "devId = pBleFinder->PromptUserForDevice(); = " << devId.c_str();
	}

	httplib::Client cli("localhost", 8080);
	pCli = &cli;

	if (devId != L"" || deviceAddress != 0) {
		try {
			Windows::Devices::Bluetooth::BluetoothLEDevice bleDev = nullptr;
			if (devId != L"") {
				// Connect to the device by the Id
				// Creating a BluetoothLEDevice object by calling this method alone doesn't (necessarily) initiate a connection.
				bleDev = BluetoothLEDevice::FromIdAsync(devId).get();
			}
			else {
				// Connect to the device by the adress
				bleDev = BluetoothLEDevice::FromBluetoothAddressAsync(deviceAddress).get();
			}

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
						else if (result.get().Status() == GattCommunicationStatus::Success) {
							// Should only be one, but we get a vector, lets iterate						
							for (GattCharacteristic c : result.get().Characteristics()) {
								wcout << "Found matching GattCharacteristic " << to_hstring(c.Uuid()).c_str() << endl
									<< "Press enter to start subscribing to notifications, press enter again to stop:\n";
								cin.getline(userInput, 2);

								//
								// Check that characteristic is writable. Themn write to it to tell the dev that we want notifications								
								//
								if (GattCharacteristicProperties::None != (c.CharacteristicProperties() & GattCharacteristicProperties::Notify)) {
									event_token t = c.ValueChanged(stylusValueHandler);
									c.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
									cin.getline(userInput, 2);
								}
							}
						}
						else {
							wcout << "Com error" << (int)result.GetResults().Status() << "\n";
						}
					}
				}
			}
			bleDev.Close();
		}
		catch (...) {
			wcout << "\nError or device connection lost.\n";
		}
	}
	wcout << "Enter to quit";
	cin.getline(userInput, 2);

	pCli = NULL; // Indicate to any late threads taht the HTTP client is now invalid.

	return 0;
}

