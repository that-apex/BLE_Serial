#ifndef BLE_SERIAL_SRC_PLATFORM_WINDOWS_BLUETOOTH_HPP_
#define BLE_SERIAL_SRC_PLATFORM_WINDOWS_BLUETOOTH_HPP_

#include <ble_serial/bluetooth.hpp>

#include <string>
#include <utility>
#include <vector>

#include <windows.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

namespace BLE_Serial::Bluetooth
{
    using namespace winrt::Windows::Devices::Bluetooth;
    using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
    using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
    using namespace winrt::Windows::Devices::Enumeration;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Storage::Streams;

    // Forward declarations
    class WindowsBluetoothDevice;
    class WindowsBluetoothConnection;
    class WindowsBluetoothGattService;
    class WindowsBluetoothGattCharacteristic;

    /**
     * IBluetoothService implementation using the Windows BLE API.
     */
    class WindowsBluetoothService : public IBluetoothService
    {
    private:
        template<typename Notify>
        static BluetoothLEAdvertisementWatcher CreateDeviceWatcher(Notify &&notify);

    public:
        void Initialize() override;

        BluetoothUUID UUIDFromString(std::string_view string) override;

        std::string UUIDToString(BluetoothUUID uuid) override;

        std::string UUIDToShortString(BluetoothUUID uuid) override;

        void ScanDevices(std::vector<std::unique_ptr<IBluetoothDevice>> &output, std::chrono::seconds timeout) override;

        std::optional<std::unique_ptr<IBluetoothDevice>> FindDevice(BluetoothAddress address, std::chrono::seconds timelimit) override;
    };


    /**
     * IBluetoothDevice implementation using the Windows BLE API.
     */
    class WindowsBluetoothDevice : public IBluetoothDevice
    {
    public:
        WindowsBluetoothDevice(BluetoothAddress deviceAddress, std::wstring deviceName);

        [[nodiscard]] BluetoothAddress GetDeviceAddress() const noexcept override;

        [[nodiscard]] const std::wstring &GetDeviceName() const noexcept override;

        [[nodiscard]] std::optional<std::shared_ptr<IBluetoothConnection>> GetOpenConnection() const noexcept override;

        [[nodiscard]] std::shared_ptr<IBluetoothConnection> OpenConnection(std::chrono::seconds timeout) override;

    private:
        BluetoothAddress m_deviceAddress;
        std::wstring m_deviceName;
        std::shared_ptr<WindowsBluetoothConnection> m_openConnection;
    };

    /**
     * IBluetoothConnection implementation using the Windows BLE API.
     */
    class WindowsBluetoothConnection : public IBluetoothConnection
    {
    public:
        WindowsBluetoothConnection(BluetoothLEDevice device, std::chrono::seconds timeout, const IVectorView<GattDeviceService> &services);

        [[nodiscard]] bool IsOpen() const noexcept override;

        void Close() override;

        [[nodiscard]] std::vector<std::unique_ptr<IBluetoothGattService>> &GetServices() override;

        std::unique_ptr<IBluetoothGattService> &GetService(BluetoothUUID uuid) override;

    private:
        std::chrono::seconds m_timeout;
        BluetoothLEDevice m_device;
        std::vector<std::unique_ptr<IBluetoothGattService>> m_services;
    };

    /**
     * IBluetoothGattService implementation using the Windows BLE API.
     */
    class WindowsBluetoothGattService : public IBluetoothGattService
    {
    public:
        WindowsBluetoothGattService(GattDeviceService service, std::chrono::seconds timeout);

        [[nodiscard]] BluetoothUUID GetUUID() const override;

        [[nodiscard]] GattRegisteredService GetRegisteredServiceType() const override;

        std::vector<std::unique_ptr<IBluetoothGattCharacteristic>> &GetCachedCharacteristics() override;

        std::unique_ptr<IBluetoothGattCharacteristic> &GetCharacteristic(BluetoothUUID uuid) override;

        void FetchCharacteristics() override;

    private:
        GattDeviceService m_service;
        std::chrono::seconds m_timeout;
        BluetoothUUID m_uuid;
        std::vector<std::unique_ptr<IBluetoothGattCharacteristic>> m_characteristics {};
    };

    /**
     * IBluetoothGattCharacteristic implementation using the Windows BLE API.
     */
    class WindowsBluetoothGattCharacteristic : public IBluetoothGattCharacteristic
    {
    public:
        WindowsBluetoothGattCharacteristic(GattCharacteristic characteristic, std::chrono::seconds timeout);

        [[nodiscard]] BluetoothUUID GetUUID() const override;

        [[nodiscard]] GattRegisteredCharacteristic GetRegisteredCharacteristicType() const override;

        std::vector<uint8_t> Read() override;

        void Write(const std::vector<uint8_t> &data) override;

        size_t Subscribe(std::function<void(std::vector<uint8_t>)> listener) override;

        void Unsubscribe(size_t id) override;

        void UnsubscribeAll() override;

    private:
        GattCharacteristic m_characteristic;
        std::chrono::seconds m_timeout;
        BluetoothUUID m_uuid;
        std::vector<winrt::event_token> m_subscribers;
    };

}

#endif //BLE_SERIAL_SRC_PLATFORM_WINDOWS_BLUETOOTH_HPP_
