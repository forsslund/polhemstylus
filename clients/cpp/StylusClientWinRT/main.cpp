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

struct StylusData {
	bool button;
	uint16_t rotation;
};

fire_and_forget myHandler(GattCharacteristic c, GattValueChangedEventArgs const& v) {	
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

constexpr guid stylusService					= { 0x90ad0000, 0x662b, 0x4504, { 0xb8, 0x40, 0x0f, 0xf1, 0xdd, 0x28, 0xd8, 0x4e } };
constexpr guid stylusValueCharacteristicUUID	= { 0x90ad0001, 0x662b, 0x4504, { 0xb8, 0x40, 0x0f, 0xf1, 0xdd, 0x28, 0xd8, 0x4e } };

int main()
{
    init_apartment();
	char userInput[3];
	userInput[0] = '\0';


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

			while (userInput[0] != 'q' && userInput[0] != 'Q' && userInput[0] != 'c') {
				// Get services of the device
				auto gattServices{ bleDev.GetGattServicesAsync(BluetoothCacheMode::Uncached).get() };

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
				wcout << endl << "Enter to read again, C continue to notifications from our characteristic, Q to quit:";
				cin.getline(userInput, 2);
			}
			if (userInput[0] == 'c' || userInput[0] == 'C')
			{
				wcout << "Continue..." << endl;
				
				auto gattServices{ bleDev.GetGattServicesAsync(BluetoothCacheMode::Uncached).get() };
				for (GattDeviceService service : gattServices.Services()) {
					GattCommunicationStatus gattCommunicationStatus = gattServices.Status();
					if (gattCommunicationStatus == GattCommunicationStatus::Success) {
						if (service.Uuid() == stylusService) {
							wcout << "\nFound our service...\n";
							auto result = service.GetCharacteristicsForUuidAsync(stylusValueCharacteristicUUID);
							// Let the com thread have some time to communicate
							while (!(result.Completed() || result.ErrorCode() || i < 100)) {
								_sleep(100);
							}
							if (result.ErrorCode()) {
								wcout << "GetCharacteristicsForUuidAsync(stylusValueCharacteristicUUID) got error code " << result.ErrorCode() << "\n";
							}
							else if (result.get().Status() == GattCommunicationStatus::Success){
								// Should only be one, but we get a vector, lets iterate
								i = 0;
								// GattCharacteristic c = result.Characteristics().GetAt(0);
								for (GattCharacteristic c : result.get().Characteristics()) {
									wcout << "  Found matching GattCharacteristic, requesting notifications. " << i << ":\t";
									wcout << GetCharacteristicName(c).c_str() << endl;									
									if (i == 0 && (c.CharacteristicProperties() & GattCharacteristicProperties::Notify) != GattCharacteristicProperties::None) {
										event_token t = c.ValueChanged(myHandler);
										c.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
										cin.getline(userInput, 2);
									}
									i++;
								}
								wcout << "\n\n";								
							}
							else {
								wcout << "Com error"<< (int) result.GetResults().Status()<<"\n";
							}
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

