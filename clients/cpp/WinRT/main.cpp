#include "pch.h"
#include "BLEdeviceFinder.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Storage.Streams.h"
#include "socketServer.h"

using namespace winrt;
using namespace Windows::Foundation;

BLEdeviceFinder* BLEdeviceFinder::instance = 0;

#include <iostream>
using namespace Windows::Storage::Streams;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;

constexpr guid stylusService			 = { 0x90ad0000, 0x662b, 0x4504, { 0xb8, 0x40, 0x0f, 0xf1, 0xdd, 0x28, 0xd8, 0x4e } };
constexpr guid stylusValueCharacteristic = { 0x90ad0001, 0x662b, 0x4504, { 0xb8, 0x40, 0x0f, 0xf1, 0xdd, 0x28, 0xd8, 0x4e } };

bool verbose = true;

SocketServer server;

fire_and_forget stylusValueHandler(GattCharacteristic c, GattValueChangedEventArgs const& v) {		
	uint16_t data = *((uint16_t*) &v.CharacteristicValue().data()[0]);
	server.Send(data);	
	return winrt::fire_and_forget();
}


int main(int argc, char* argv[])
{
	init_apartment();	
	uint64_t deviceAddress = 0;
	string url = "/dev/stylus1";
	if (argc == 3) {
		// first arg is dev id
		try{
			deviceAddress = stoll(argv[1], 0, 16);	// Convert adress string in hex format
			wcout << "Connect to device with address 0x" << std::hex << deviceAddress << std::dec << endl;
		}
		catch (...) {
			deviceAddress = 0;
		}

		// second arg is URL
		url = argv[2];

	}

	if (argc !=1 && deviceAddress == 0) {
		wcout << "Usage\t" << argv[0] << "\t" << "\t\t\t" << "Interactive device selection and test connection" << endl
			<< "\t" << argv[0] << " deviceId url" << "\t\t" << "Connect to device by deviceId (hex) and send to url" << endl << endl
			<< "Example\t" << argv[0] << " 0xc56154495792 http://localhost:8080/set" << "\t\t" << endl;
		return 1;
	}

	char userInput[3];
	userInput[0] = '\0';
	hstring devId= L"";
	if (deviceAddress == 0) {
		// No adress given, prompt user
		BLEdeviceFinder* pBleFinder = BLEdeviceFinder::getInstance();
		devId = pBleFinder->PromptUserForDevice();		
	}

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
				<< "BluetoothAddress: 0x" << std::hex << bleDev.BluetoothAddress() << endl
				<< "DeviceAccessInformation: " << (uint16_t)bleDev.DeviceAccessInformation().CurrentStatus() << endl;

			//
			// Search device for the service we want
			//
			bool found = false;
			GattCharacteristic selectedCharacteristic = nullptr;
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
							found = true;
							selectedCharacteristic = result.get().Characteristics().GetAt(0);
							if(selectedCharacteristic!=nullptr){
								wcout << "Found matching GattCharacteristic " << to_hstring(selectedCharacteristic.Uuid()).c_str() << endl;
								//
								// Check that characteristic is writable. Then write to it to tell the dev that we want notifications								
								//
								if (GattCharacteristicProperties::None != (selectedCharacteristic.CharacteristicProperties() & GattCharacteristicProperties::Notify)) {
									event_token t = selectedCharacteristic.ValueChanged(stylusValueHandler);
									selectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);									
								}
							}
						}
					}		
				}
			}			
			if(!found)
			{
				wcout << "Device not Polhem Stylus compatible." << endl;
				return 1;
			}
			wcout << "Starting server "<< url.c_str() <<endl;
			server.Start(url);

			wcout << "Enter to quit";
			cin.getline(userInput, 2);
			server.Shutdown();
			wcout << "Closing BLE...";			
			bleDev.Close();
		
		}
		catch (...) {
			wcout << "\nError or device connection lost.\n";
		}
	}
			
	return 0;
}

