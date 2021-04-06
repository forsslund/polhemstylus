#include "pch.h"
#include "BLEdeviceFinder.h"
#include <iostream>

BLEdeviceFinder* BLEdeviceFinder::getInstance()
{
	if (!instance)
		instance = new BLEdeviceFinder;
	return instance;
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_Added(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformation deviceInfo)
{	
	// Ignore if no name
	if (!deviceInfo.Name().empty())
	{
		// Ignore if already in list
		for (auto d : getInstance()->devices) {
			if (d.Id() == deviceInfo.Id()) {
				return winrt::fire_and_forget();
			}
		}
		getInstance()->devices.push_back(deviceInfo);
		wcout << "[" << getInstance()->devices.size() << "] " << (std::wstring_view) deviceInfo.Name()
			   << " " << (std::wstring_view) deviceInfo.Id()  << endl;
	}
	return winrt::fire_and_forget();
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_Updated(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate)
{
	if (getInstance()->verbose) {
		wcout << "DeviceWatcher_Updated\t" << (std::wstring_view) deviceInfoUpdate.Id() << endl;
	}
	for (const auto& devInfo : getInstance()->devices) {
		if (devInfo.Id() == deviceInfoUpdate.Id()) {
			devInfo.Update(deviceInfoUpdate);
			break;
		}
	}
	return winrt::fire_and_forget();
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_Removed(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate)
{
	if (getInstance()->verbose) {
		wcout << "DeviceWatcher_Removed\t" << (std::wstring_view) deviceInfoUpdate.Id();
	}
	for(size_t i=0;i< getInstance()->devices.size(); i++) {
		if (getInstance()->devices[i].Id() == deviceInfoUpdate.Id()) {
			wcout << "["<<i+1<<"] No longer valid\t" << (std::wstring_view) deviceInfoUpdate.Id()<<endl;
			break;
		}
	}
	return winrt::fire_and_forget();
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_EnumerationCompleted(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&)
{
	cout << "Enumeration complete\n";
	getInstance()->enumerationComplete = true;
	return winrt::fire_and_forget();
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_Stopped(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&)
{
	cout << "Enumeration stopped\n";
	return winrt::fire_and_forget();
}

hstring BLEdeviceFinder::PromptUserForDevice()
{
	// BT_Code: Scan for paired and non-paired in a single query.
	hstring aqsAllBluetoothLEDevices = L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")";
	auto requestedProperties = single_threaded_vector<hstring>({ L"System.Devices.Aep.DeviceAddress", L"System.Devices.Aep.IsConnected", L"System.Devices.Aep.Bluetooth.Le.IsConnectable" });

	DeviceWatcher deviceWatcher =
		DeviceInformation::CreateWatcher(
			aqsAllBluetoothLEDevices,
			requestedProperties,
			DeviceInformationKind::AssociationEndpoint);

	// Register event handlers before starting the watcher. Ola might want to make the referenced callbacks into weak references https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/weak-references.
	// Added, Updated and Removed are required to get all nearby devices
	event_token deviceWatcherAddedToken = deviceWatcher.Added(DeviceWatcher_Added);
	event_token deviceWatcherUpdatedToken = deviceWatcher.Updated(DeviceWatcher_Updated);
	event_token deviceWatcherRemovedToken = deviceWatcher.Removed(DeviceWatcher_Removed);

	// EnumerationCompleted and Stopped are optional to implement.
	event_token deviceWatcherEnumerationCompletedToken = deviceWatcher.EnumerationCompleted(&BLEdeviceFinder::DeviceWatcher_EnumerationCompleted);
	event_token deviceWatcherStoppedToken = deviceWatcher.Stopped(&BLEdeviceFinder::DeviceWatcher_Stopped);

	//
	// Start the watcher, and wait for enumeration to complete
	//
	cout << "Starting watcher...";
	deviceWatcher.Start();
	cout << " started.\nEnumerating... select device number, or 0 to quit\n\n";

	char userInput[3];
	userInput[0] = '\0';	
	size_t i = 1;
	cin.getline(userInput, 2);
	i = atoi(userInput);

	deviceWatcher.Stop();
	deviceWatcher.Added(deviceWatcherAddedToken);
	deviceWatcher.Updated(deviceWatcherUpdatedToken);
	deviceWatcher.Removed(deviceWatcherRemovedToken);
	deviceWatcher.EnumerationCompleted(deviceWatcherEnumerationCompletedToken);
	deviceWatcher.Stopped(deviceWatcherStoppedToken);

	if (0 < i && i <= devices.size()) {
		return devices.at(i - 1).Id();
	}
	return L"";
}

void BLEdeviceFinder::ListDevices()
{
	for (auto dev : devices) {
		std::wcout << (wstring_view)dev.Id() << " "
			<< (wstring_view)dev.Name() << "\n";
	}
}

vector<Windows::Devices::Enumeration::DeviceInformation> BLEdeviceFinder::GetDevices()
{
	return devices;
}
