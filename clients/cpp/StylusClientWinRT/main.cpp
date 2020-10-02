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
#include "helpers.h"
using namespace helpers;

int main()
{
    init_apartment();

   // BLEdeviceFinder* pBleFinder = pBleFinder->getInstance();
	BLEdeviceFinder* pBleFinder = BLEdeviceFinder::getInstance();
    size_t i = pBleFinder->Enumerate();
	wcout << endl;
	Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLeDevice{ nullptr };
	if (0 < i && i <= pBleFinder->GetDevices().size()) {
		try {
			auto dev = pBleFinder->GetDevices().at(i - 1);
			wcout << unbox_value<hstring>(dev.Properties().TryLookup(L"System.Devices.Aep.DeviceAddress")).c_str() << endl;
			//IAsyncOperation< Windows::Devices::Bluetooth::BluetoothLEDevice > a;
			//a = BluetoothLEDevice::FromIdAsync(dev.Id());

			// Connect to the device by the Id
			// Creating a BluetoothLEDevice object by calling this method alone doesn't (necessarily) initiate a connection.
			Windows::Devices::Bluetooth::BluetoothLEDevice bleDev{ BluetoothLEDevice::FromIdAsync(dev.Id()).get() };

			if (bleDev == nullptr)
			{
				wcout << "Failed to connect to device." << endl;
				return 1;
			}
			wcout << "Name: " << bleDev.Name().c_str() << endl;
			wcout << "Connection status:" << (bleDev.ConnectionStatus() == BluetoothConnectionStatus::Connected ? "Connected" : "Not Connected") << endl;
			wcout << "BluetoothAddress: " << std::hex << bleDev.BluetoothAddress() << endl;
			wcout << "DeviceAccessInformation: " << (uint16_t)bleDev.DeviceAccessInformation().CurrentStatus() << endl;
			
			// Get services of the device
			auto gattServices{ bleDev.GetGattServicesAsync(BluetoothCacheMode::Uncached).get() };

			
			char userInput[3];
			userInput[0] = '\0';
			while( userInput[0] != 'q' && userInput[0] != 'Q')  {	
				wcout << "\n\n===== Exploring device =====" << endl;
				GattCommunicationStatus gattCommunicationStatus = gattServices.Status();
				if (gattCommunicationStatus == GattCommunicationStatus::Success) {
					for (GattDeviceService service : gattServices.Services()) {
						wcout << "\nService uuid " << std::hex << service.Uuid().Data1 << "-" << service.Uuid().Data2 << "-" << service.Uuid().Data3 << "-"
							<< (uint8_t)service.Uuid().Data4[0] << " "
							<< (uint8_t)service.Uuid().Data4[1] << " "
							<< (uint8_t)service.Uuid().Data4[2] << " "
							<< (uint8_t)service.Uuid().Data4[3] << " "
							<< (uint8_t)service.Uuid().Data4[4] << " "
							<< (uint8_t)service.Uuid().Data4[5] << " "
							<< (uint8_t)service.Uuid().Data4[6] << " "
							<< (uint8_t)service.Uuid().Data4[7] << " ";
						wcout << GetServiceName(service).c_str() << endl;
						i = 0;
						for (GattCharacteristic c : service.GetAllCharacteristics()) {
							wcout << "  GattCharacteristic " << i++ << ":\t";
							wcout << GetCharacteristicName(c).c_str() << endl;
							wcout << "    ";

							GattCharacteristicProperties p = c.CharacteristicProperties();
							if ((p & GattCharacteristicProperties::Notify | p & GattCharacteristicProperties::Indicate) != GattCharacteristicProperties::None) {
								cout << "Supports notifications. ";
							}
							if ((p & GattCharacteristicProperties::Write | GattCharacteristicProperties::WriteWithoutResponse) != GattCharacteristicProperties::None) {
								wcout << "Writeable ";
							}
							if ((p & GattCharacteristicProperties::Read) != GattCharacteristicProperties::None) {
								GattReadResult g = c.ReadValueAsync(BluetoothCacheMode::Uncached).get();
								if (g.Status() == GattCommunicationStatus::Success) {
									wcout << "Readable, Data : ";
									IBuffer myBuffer = g.Value();
									for (uint32_t j = 0; j < myBuffer.Length(); ++j) {
										wcout << " " << myBuffer.data()[j] << " ";
									}
								}
								else {
									wcout << "Readable, but read failed";
								}
							}
							wcout << endl;
						}					
					}
				}
				else {
					switch (gattServices.Status()) {
					case GattCommunicationStatus::AccessDenied: wcout << "Access is denied." << endl; break;
					case GattCommunicationStatus::ProtocolError: wcout << "There was a GATT communication protocol error." << endl; break;
					case GattCommunicationStatus::Unreachable: wcout << "No communication can be performed with the device, at this time." << endl; break;
					default: wcout << "gattCommunicationStatus=" << (int)gattCommunicationStatus << endl;
					}
				}								
				wcout << endl << "Enter to read again, Q to quit:";
				cin.getline(userInput, 2);
				_sleep(250);
			} 

			// In documentatoin https://docs.microsoft.com/en-us/windows/uwp/devices-sensors/gatt-client they do (C#)
			// bluetoothLeDevice.Dispose();
			// But this member does not exist. Now its stricly not required as it will timeout, but anoying if you want to stay clean
		}
		catch(...){
			wcout << "\nError or device connection lost.\n";
		}
	}
	wcout << endl;
	

	return 0;

}

