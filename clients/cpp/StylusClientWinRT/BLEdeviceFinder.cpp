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
	wcout << "DeviceWatcher_Added\t" << (std::wstring_view) deviceInfo.Id()
		<< " " << (std::wstring_view) deviceInfo.Name() << "\n";
	getInstance()->devices.push_back(deviceInfo);
	return winrt::fire_and_forget();
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_Updated(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate)
{
	wcout << "DeviceWatcher_Updated\t" << (std::wstring_view) deviceInfoUpdate.Id();
	for (const auto& devInfo : getInstance()->devices) {
		if (devInfo.Id() == deviceInfoUpdate.Id()) {
			if (devInfo.Name().empty()) {
				cout << " adding name: ";
				devInfo.Update(deviceInfoUpdate);
			}
			wcout << " " << (std::wstring_view) devInfo.Name();
			break;
		}
	}

	cout << endl;
	return winrt::fire_and_forget();
}

fire_and_forget BLEdeviceFinder::DeviceWatcher_Removed(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate)
{
	wcout << "DeviceWatcher_Removed\t" << (std::wstring_view) deviceInfoUpdate.Id();
	for (auto iter = getInstance()->devices.begin(); iter != getInstance()->devices.end(); iter++) {
		if (iter->Id() == deviceInfoUpdate.Id()) {
			wcout << " " << (std::wstring_view) iter->Name() << " was found in list and removed.\n";
			getInstance()->devices.erase(iter);
			return winrt::fire_and_forget();
		}
	}
	cout << " was not in our list!" << endl;

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

void BLEdeviceFinder::Enumerate() {

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
	event_token deviceWatcherAddedToken   = deviceWatcher.Added(   DeviceWatcher_Added);
	event_token deviceWatcherUpdatedToken = deviceWatcher.Updated( DeviceWatcher_Updated);
	event_token deviceWatcherRemovedToken = deviceWatcher.Removed( DeviceWatcher_Removed);

	// EnumerationCompleted and Stopped are optional to implement.
	event_token deviceWatcherEnumerationCompletedToken = deviceWatcher.EnumerationCompleted(&BLEdeviceFinder::DeviceWatcher_EnumerationCompleted);
	event_token deviceWatcherStoppedToken = deviceWatcher.Stopped(&BLEdeviceFinder::DeviceWatcher_Stopped);

	//
	// Start the watcher, and wait for enumeration to complete
	//
	cout << "Starting watcher...";
	deviceWatcher.Start();
	cout << " started.\nEnumerating...\n\n";
	while (!enumerationComplete);

	//
	// Stop watcher
	// Accordign to soem online examples, unregister the callbacks before stopping might mean they cant be called after we have stoped, could avoid async problems
	// if callbacks have deps according to some example. Seems ugly imho thought.
	//
	deviceWatcher.Added(deviceWatcherAddedToken);
	deviceWatcher.Updated(deviceWatcherUpdatedToken);
	deviceWatcher.Removed(deviceWatcherRemovedToken);
	deviceWatcher.EnumerationCompleted(deviceWatcherEnumerationCompletedToken);
	deviceWatcher.Stopped(deviceWatcherStoppedToken);

	cout << "\n\n...enumeration stopped.\n";
	deviceWatcher.Stop();
	_sleep(250);	// Just to see if we do get latecommers (we do, if we dont unregister before stop). Evidently, we do anyway, async protection needs to be in callbacks.
	cout << "\n\nThese " << devices.size() << " devices were found:" << endl << endl;
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
