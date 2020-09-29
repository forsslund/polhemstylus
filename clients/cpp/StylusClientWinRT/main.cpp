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


int main()
{
    init_apartment();

    BLEdeviceFinder* pBleFinder = pBleFinder->getInstance();
    
    pBleFinder->Enumerate();
	
	wcout << "Enumerate BLE devices." << endl;
	size_t i = 1;
	for (auto dev : pBleFinder->GetDevices()) {
		wcout << "[" << i++ << "]\t";
		wcout << (wstring_view)dev.Id() << " "
			  << (wstring_view)dev.Name() << "\n";		
		//wcout << unbox_value<hstring>dev.Properties().TryLookup(L"System.Devices.Aep.DeviceAddress").c_str();
		
	}

	wcout<<"\n\nSelect device nr, use 0 to quit.";
	cin >> i;
	
	Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLeDevice{ nullptr };
	if (0 < i && i <= pBleFinder->GetDevices().size()) {
		auto dev = pBleFinder->GetDevices().at(i - 1);
		wcout << unbox_value<hstring>(dev.Properties().TryLookup(L"System.Devices.Aep.DeviceAddress")).c_str();
		IAsyncOperation< Windows::Devices::Bluetooth::BluetoothLEDevice > a;
		a = BluetoothLEDevice::FromIdAsync(dev.Id());


		Windows::Devices::Bluetooth::BluetoothLEDevice bleDev{ BluetoothLEDevice::FromIdAsync(dev.Id()).get() };

		if (bleDev == nullptr)
		{
			wcout << "Failed to connect to device." << endl;
			return 1;
		}
		wcout << "Name: " << bleDev.Name().c_str() << endl;
		wcout << "Connection status:" << (bleDev.ConnectionStatus() == BluetoothConnectionStatus::Connected ? "Connected" : "Not Connected") << endl;
		wcout << "BluetoothAddress: " << bleDev.BluetoothAddress() << endl;
		wcout << "DeviceAccessInformation: " << (uint16_t)bleDev.DeviceAccessInformation().CurrentStatus() << endl;

		/*wcout << "Paring..." << endl;
		DevicePairingResult result = bleDev.DeviceInformation().Pairing().PairAsync().get();

		switch (result.Status()){
			case DevicePairingResultStatus::Paired: wcout << "Paired" << endl; break;
			case DevicePairingResultStatus::AlreadyPaired: wcout << "AlreadyPaired" << endl; break;
			default: wcout << "what now? " << (int)result.Status() <<endl;
		}*/

		auto gattServices{ bleDev.GetGattServicesAsync(BluetoothCacheMode::Uncached).get() };

		wcout << "gattServices.Status()=" << (int)gattServices.Status() << endl;
		switch (gattServices.Status()) {
		case GattCommunicationStatus::AccessDenied: wcout << "Access is denied." << endl; break;
		case GattCommunicationStatus::ProtocolError: wcout << "There was a GATT communication protocol error." << endl; break;
		case GattCommunicationStatus::Success: wcout << "The operation completed successfully." << endl; break;
		case GattCommunicationStatus::Unreachable: wcout << "No communication can be performed with the device, at this time." << endl; break;
		}
		string input;
		do{
			for (GattDeviceService service : gattServices.Services()) {

				wcout << "\nService uuid " << service.Uuid().Data1 << " " << service.Uuid().Data2 << " " << service.Uuid().Data3 << " " << (uint8_t)service.Uuid().Data4 <<endl;
				i = 0;
				for (GattCharacteristic c : service.GetAllCharacteristics()) {
					wcout << "GattCharacteristic " << i++ << ":\t";
					GattReadResult g = c.ReadValueAsync(BluetoothCacheMode::Uncached).get();
				
					IBuffer myBuffer = g.Value();
					for (uint32_t j = 0; j < myBuffer.Length(); ++j) {
						wcout << " " << myBuffer.data()[j] << " ";
					}
					wcout << endl;
				}
			}
			wcout << "Press Q to quit." << endl;
			cin >> input;
		} while ( !input.compare("q") && !input.compare("Q"));
	}
	wcout << endl;
	

	return 0;

}

