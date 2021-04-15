#include "pch.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Storage.Streams.h"

#include "socketServer.h"
#include "BLEdeviceFinder.h"
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

// BLE callback handler for recieving data
fire_and_forget stylusValueHandler(GattCharacteristic c, GattValueChangedEventArgs const& v) {		
	uint16_t data = *((uint16_t*) &v.CharacteristicValue().data()[0]);
	server.Send(data);	
	return winrt::fire_and_forget();
}

// BLE callback handler to keep track of session status (connected or not)
GattSessionStatus sessionStatus = GattSessionStatus::Active;
fire_and_forget stylusSessionStatusChanged(GattSession s, GattSessionStatusChangedEventArgs const& a) {
	sessionStatus = a.Status();
	std::wcout << "stylusSessionStatusChanged "
		<< "\ta.Status() " << ((a.Status()== GattSessionStatus::Active) ? "GattSessionStatus::Active" : "GattSessionStatus::Closed") << endl
		<< "\ta.Error() " << ( a.Error() == BluetoothError::Success ? "BluetoothError::Success" : "BluetoothError") << endl;

	return winrt::fire_and_forget();
}


int main(int argc, char* argv[])
{	
	uint64_t deviceAddress = 0;
	std::string url = "/dev/stylus1";
	std::string programName = argv[0];	
	// Clean up program name
	if (programName.find_last_of("/\\") != std::string::npos) {
		programName = programName.substr(1+ programName.find_last_of("/\\"));
	}
		
	if (argc == 3) {
		// first arg is dev id
		try{
			deviceAddress = stoll(argv[1], 0, 16);	// Convert adress string in hex format
			std::wcout << "Device to use: 0x" << std::hex << deviceAddress << std::dec << endl;
		}
		catch (...) {
			deviceAddress = 0;
			wcout << "Could not interprete device address " << argv[1] << endl;
		}

		// second arg is URL
		url = argv[2];

	}
	else if(argc==2){
		deviceAddress = 0;
		url = argv[1];
	}

	if ((argc != 2 && argc != 3) ||(argc == 3 && deviceAddress == 0) ) {
		cout << "Usage:\n" 
			<< "\t" << programName << " path        " << "\t" << "Interactive device selection and send stylus data on socket path" << endl			  
			<< "\t" << programName << " address path" << "\t" << "Connect to device by address (hex) and send stylus data on socket path" << endl
			<< endl
			<< "Example:\t" << programName << " 0xC56154495792 " << url << endl;
		return 1;
	}

	init_apartment();
	if (deviceAddress == 0) {
		// No adress given, prompt user
		hstring devId = L"";
		BLEdeviceFinder* pBleFinder = BLEdeviceFinder::getInstance();
		devId = pBleFinder->PromptUserForDevice();
		if (devId != L"") {
			deviceAddress = BluetoothLEDevice::FromIdAsync(devId).get().BluetoothAddress();
		}		
	}
	
	if ( deviceAddress != 0) {
		wcout << "Stylus data will be sent to: " << url.c_str() << endl
			<< "Ctrl-c to quit." << endl;
		server.Start(url);
		bool quit = false;
		while(!quit){
			if (server.HasActiveClient()) {				
				try {
					Windows::Devices::Bluetooth::BluetoothLEDevice bleDev = nullptr;
	
					// Connect to the device by the adress
					bleDev = BluetoothLEDevice::FromBluetoothAddressAsync(deviceAddress).get();			

					if (bleDev != nullptr) {
						event_token sessionStatusChangedToken;
						event_token stylusHandlerToken;
						GattCharacteristic selectedCharacteristic = nullptr;

						//
						// Search device for the service we want, and register callbacks to handle notification and connection status
						//						
						bool found = false;
						auto gattServices{ bleDev.GetGattServicesAsync(BluetoothCacheMode::Uncached).get() };
						for (GattDeviceService service : gattServices.Services()) {
							GattCommunicationStatus gattCommunicationStatus = gattServices.Status();							
							if (gattCommunicationStatus == GattCommunicationStatus::Success) {
								sessionStatus = GattSessionStatus::Active;
								if (service.Uuid() == stylusService) {
									IAsyncOperation<GattCharacteristicsResult> result = service.GetCharacteristicsForUuidAsync(stylusValueCharacteristic);
																		
									// Register callback to keep track of session status
									service.Session().SessionStatusChanged(stylusSessionStatusChanged);

									//
									// Let the com thread have some time to communicate
									//
									size_t i = 0;
									while (!(result.Completed() || result.ErrorCode() || ++i < 100)) {
										Sleep(100);										
									}

									if (result.ErrorCode()) {
										wcout << "GetCharacteristicsForUuidAsync(stylusValueCharacteristic) got error code " << result.ErrorCode() << ".\n";
									}
									else
									{
										GattCharacteristicsResult characteristicResult = result.get();
										if (characteristicResult.Status() == GattCommunicationStatus::Success) {
											found = true;
											selectedCharacteristic = characteristicResult.Characteristics().GetAt(0);
											if (selectedCharacteristic != nullptr) {
												if (verbose) wcout << "Found matching GattCharacteristic " << to_hstring(selectedCharacteristic.Uuid()).c_str() << endl;
												//
												// Check that characteristic is writable. Then write to it to tell the devcie that we want notifications								
												//
												if (GattCharacteristicProperties::None != (selectedCharacteristic.CharacteristicProperties() & GattCharacteristicProperties::Notify)) {
													stylusHandlerToken = selectedCharacteristic.ValueChanged(stylusValueHandler);
													selectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
												}
											}
										}
									}
								}
							}
						}						
						if (!found)
						{
							if(verbose)	wcout << "Device not Polhem Stylus compatible." << endl;
						}
						else {
							wcout << "Connected to Stylus 0x" << std::hex << bleDev.BluetoothAddress() << endl;
							while (gattServices.Status() == GattCommunicationStatus::Success && server.HasActiveClient() && sessionStatus == GattSessionStatus::Active) {
								Sleep(100);
							}
							// TODO: Check if user wants to quit, but only when client disconnected

							// Cleanup event handlers
							if (selectedCharacteristic != nullptr) {
								selectedCharacteristic.ValueChanged(stylusHandlerToken);
							}
							//TODO sessionStatusChangedToken;
							
						}
						if (!server.HasActiveClient()) {
							wcout << "No client, disconnecting from Stylus." << endl;
						}
						else {
							wcout << "Stylus communication interrupted." << endl;
						}
						bleDev.Close();
					}
					else {
						wcout << "Failed to connect to Stylus 0x" << std::hex << deviceAddress << ". Retrying..." << endl;
					}

				}
				catch (...) {
					wcout << "\nError or device connection lost.\n";
				}				
			}
			/*
			int c = std::cin.peek();
			if (c != EOF) {
				string str;
				cin >> str;
				if( str.find("q") != string::npos || str.find("Q") != string::npos) quit = true;
			}
			else {
				Sleep(1000);	// Give it some slack before trying again
			}*/			
			Sleep(1000);	// Give it some slack before trying again
		}
	}
	return 0;
}

