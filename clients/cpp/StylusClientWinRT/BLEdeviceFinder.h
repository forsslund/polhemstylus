#pragma once
#include "winrt/Windows.Devices.Bluetooth.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Devices.Enumeration.h"

using namespace winrt;
using namespace std;
using namespace Windows::Devices::Enumeration;

// This is a singelton class
class BLEdeviceFinder
{
	
	BLEdeviceFinder() { instance = NULL; } // Private constructor so that no objects can be created.
	static BLEdeviceFinder* instance;

	
	vector<Windows::Devices::Enumeration::DeviceInformation> devices;
	bool enumerationComplete = false;
public:
	bool  verbose = false;
	static BLEdeviceFinder* getInstance();
	static fire_and_forget DeviceWatcher_Added(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformation deviceInfo);
	static fire_and_forget DeviceWatcher_Updated(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate);
	static fire_and_forget DeviceWatcher_Removed(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate);
	static fire_and_forget DeviceWatcher_EnumerationCompleted(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&);
	static fire_and_forget DeviceWatcher_Stopped(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&);
	hstring PromptUserForDevice();
	void ListDevices();
	vector<Windows::Devices::Enumeration::DeviceInformation> GetDevices();
};

