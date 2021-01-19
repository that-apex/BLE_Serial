#include "bluetooth.hpp"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING // ugly :(

#include <iostream>
#include <algorithm>
#include <codecvt>
#include <locale>

#ifdef _MSC_VER
#   pragma comment(lib, "windowsapp")
#endif

namespace BLE_Serial::Bluetooth
{
    namespace
    {
        static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> g_wideStringToUtf8;  // NOLINT (-Wdeprecated-declarations)

        /**
         * Helper for converting UUIDs
         */
        BluetoothUUID GUIDToBluetoothUUID(GUID guid)
        {
            static_assert(sizeof(BluetoothUUID) == sizeof(GUID), "BluetoothUUID and GUID sizes do not match");

            union
            {
                GUID guid;
                BluetoothUUID uuid;
            } helper { .guid = guid };

            return helper.uuid;
        }

        GUID GUIDFromString(std::wstring_view string)
        {
            GUID guid;
            if (CLSIDFromString(string.data(), &guid) != NOERROR) {
                throw BluetoothException("Invalid GUID");
            }

            return guid;
        }

        /**
         * Helper for blocking for winrt IAsyncOperations
         */
        template<typename R>
        static R WaitWithTimeout(IAsyncOperation<R> &&task, std::chrono::seconds timeout)
        {
            std::mutex mutex {};
            std::condition_variable condition {};

            task.Completed([&condition, &mutex](const IAsyncOperation<R>& asyncInfo, AsyncStatus asyncStatus) {
                std::unique_lock<std::mutex> lock { mutex };
                condition.notify_all();
            });

            const auto deadline = std::chrono::system_clock::now() + timeout;

            while (std::chrono::system_clock::now() < deadline && task.Status() == AsyncStatus::Started) {
                std::unique_lock<std::mutex> lock { mutex };
                condition.wait_until(lock, deadline);
            }

            switch (task.Status()) {
                case AsyncStatus::Completed:
                    return task.GetResults();
                case AsyncStatus::Error:
                    throw winrt::hresult_error(task.ErrorCode()); // NOLINT(hicpp-exception-baseclass)
                case AsyncStatus::Canceled:
                    throw BluetoothException("Operation cancelled");
                default:
                    throw BluetoothException("Operation timed out");
            }
        }
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // Macro helpers                                        //
    //                                                      //
    //////////////////////////////////////////////////////////

    #define WINRT_CALL_BEGIN \
        try

    #define WINRT_CALL_END                                                                \
        catch (const winrt::hresult_error &err) {                                         \
            std::string errorMessage = "Bluetooth error. Code: ";                         \
            errorMessage += std::to_string(err.code());                                   \
            errorMessage += ". Message: ";                                                \
            errorMessage += g_wideStringToUtf8.to_bytes(std::wstring { err.message() });  \
            throw BluetoothException(std::move(errorMessage));                            \
        }


    //////////////////////////////////////////////////////////
    //                                                      //
    // WindowsBluetoothService implementation               //
    //                                                      //
    //////////////////////////////////////////////////////////

    template<typename Notify>
    BluetoothLEAdvertisementWatcher WindowsBluetoothService::CreateDeviceWatcher(Notify &&notify)
    {
        auto factory = winrt::get_activation_factory<BluetoothLEAdvertisementWatcher, IBluetoothLEAdvertisementWatcherFactory>();
        auto filter = BluetoothLEAdvertisementFilter {};
        auto watcher = factory.Create(filter);

        watcher.Received([notify](const IBluetoothLEAdvertisementWatcher &watcher, const IBluetoothLEAdvertisementReceivedEventArgs &args) {
            auto name = args.Advertisement().LocalName();
            auto device = std::unique_ptr<IBluetoothDevice>(new WindowsBluetoothDevice { args.BluetoothAddress(), std::wstring { name.empty() ? L"(unnamed)" : name }});

            notify(std::move(device));
        });

        return watcher;
    }

    void WindowsBluetoothService::Initialize()
    {
        winrt::init_apartment();
    }

    BluetoothUUID WindowsBluetoothService::UUIDFromString(std::string_view string)
    {
        std::wstring wstring = g_wideStringToUtf8.from_bytes(string.data(), string.data() + string.size());

        if (!wstring.starts_with(L"{")) {
            wstring = L"{" + wstring + L"}";
        }

        return GUIDToBluetoothUUID(GUIDFromString(wstring));
    }

    std::string WindowsBluetoothService::UUIDToString(BluetoothUUID uuid)
    {
        char buffer[38];
        sprintf(buffer, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", uuid.custom, uuid.part2, uuid.part3, uuid.part4[0], uuid.part4[1], uuid.part4[2], uuid.part4[3], uuid.part4[4],
                uuid.part4[5], uuid.part4[6], uuid.part4[7]);
        return std::string { buffer };
    };

    std::string WindowsBluetoothService::UUIDToShortString(BluetoothUUID uuid)
    {
        char buffer[9];
        sprintf(buffer, "%08X", uuid.custom);
        return std::string { buffer };
    }

    void WindowsBluetoothService::ScanDevices(std::vector<std::unique_ptr<IBluetoothDevice>> &output, std::chrono::seconds timeout)
    {
        WINRT_CALL_BEGIN {
            auto watcher = CreateDeviceWatcher([&output](std::unique_ptr<IBluetoothDevice> device) {
                if (std::any_of(output.begin(), output.end(), [&device](const auto &current) {
                    return device->GetDeviceAddress() == current->GetDeviceAddress();
                })) {
                    // Do not duplicate devices
                    return;
                }

                output.emplace_back(reinterpret_cast<IBluetoothDevice *>(device.release()));
            });

            watcher.Start();
            std::this_thread::sleep_for(timeout);
            watcher.Stop();
        } WINRT_CALL_END;
    }

    std::optional<std::unique_ptr<IBluetoothDevice>> WindowsBluetoothService::FindDevice(BluetoothAddress address, std::chrono::seconds timelimit)
    {
        WINRT_CALL_BEGIN {
            std::optional<std::unique_ptr<IBluetoothDevice>> result;
            std::mutex mutex;
            std::condition_variable condition;

            auto watcher = CreateDeviceWatcher([&](std::unique_ptr<IBluetoothDevice> device) {
                if (device->GetDeviceAddress() != address) {
                    return;
                }

                std::unique_lock<std::mutex> lock { mutex };
                result = std::unique_ptr<IBluetoothDevice> { static_cast<IBluetoothDevice *>(device.release()) };
                condition.notify_all();
            });

            watcher.Start();

            const auto deadline = std::chrono::system_clock::now() + timelimit;

            while (std::chrono::system_clock::now() < deadline) {
                std::unique_lock<std::mutex> lock { mutex };
                condition.wait_until(lock, deadline);

                if (result) {
                    break;
                }
            }

            watcher.Stop();

            return result;
        } WINRT_CALL_END;
    }


    //////////////////////////////////////////////////////////
    //                                                      //
    // WindowsBluetoothDevice implementation                //
    //                                                      //
    //////////////////////////////////////////////////////////

    WindowsBluetoothDevice::WindowsBluetoothDevice(BluetoothAddress deviceAddress, std::wstring deviceName)
            : m_deviceAddress(deviceAddress), m_deviceName(std::move(deviceName))
    {}

    [[nodiscard]] BluetoothAddress WindowsBluetoothDevice::GetDeviceAddress() const noexcept
    {
        return m_deviceAddress;
    }

    [[nodiscard]] const std::wstring &WindowsBluetoothDevice::GetDeviceName() const noexcept
    {
        return m_deviceName;
    }

    [[nodiscard]] std::optional<std::shared_ptr<IBluetoothConnection>> WindowsBluetoothDevice::GetOpenConnection() const noexcept
    {
        if (!m_openConnection) {
            return std::nullopt;
        }

        if (!m_openConnection->IsOpen()) {
            return std::nullopt;
        }

        return m_openConnection;
    }

    [[nodiscard]] std::shared_ptr<IBluetoothConnection> WindowsBluetoothDevice::OpenConnection(std::chrono::seconds timeout)
    {
        if (m_openConnection) {
            if (m_openConnection->IsOpen()) {
                return m_openConnection;
            } else {
                m_openConnection.reset();
            }
        }

        WINRT_CALL_BEGIN {
            auto device = WaitWithTimeout(BluetoothLEDevice::FromBluetoothAddressAsync(m_deviceAddress), timeout);
            auto gattServices = WaitWithTimeout(device.GetGattServicesAsync(), timeout);
            if (gattServices.Status() != GattCommunicationStatus::Success) {
                throw BluetoothException("GetGattServicesAsync failed");
            }

            return std::make_shared<WindowsBluetoothConnection>(std::move(device), timeout, std::move(gattServices.Services()));
        } WINRT_CALL_END;
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // WindowsBluetoothConnection implementation            //
    //                                                      //
    //////////////////////////////////////////////////////////

    WindowsBluetoothConnection::WindowsBluetoothConnection(BluetoothLEDevice device, std::chrono::seconds timeout, const IVectorView<GattDeviceService> &services)
            : m_device { std::move(device) }, m_timeout { timeout }, m_services {}
    {
        m_services.reserve(services.Size());

        for (auto service : services) {
            m_services.emplace_back(std::make_unique<WindowsBluetoothGattService>(service, m_timeout));
        }
    }

    [[nodiscard]] bool WindowsBluetoothConnection::IsOpen() const noexcept
    {
        return m_device.ConnectionStatus() == BluetoothConnectionStatus::Connected;
    }

    void WindowsBluetoothConnection::Close()
    {
        WINRT_CALL_BEGIN {
            m_services.clear();
            m_device.Close();
        } WINRT_CALL_END;
    }

    [[nodiscard]] std::vector<std::unique_ptr<IBluetoothGattService>> &WindowsBluetoothConnection::GetServices()
    {
        return m_services;
    }

    std::unique_ptr<IBluetoothGattService> &WindowsBluetoothConnection::GetService(BluetoothUUID uuid)
    {
        static std::unique_ptr<IBluetoothGattService> c_nullValue {};

        for (auto &service : m_services) {
            if (service->GetUUID() == uuid) {
                return service;
            }
        }

        return c_nullValue;
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // WindowsBluetoothGattService implementation           //
    //                                                      //
    //////////////////////////////////////////////////////////

    WindowsBluetoothGattService::WindowsBluetoothGattService(GattDeviceService service, std::chrono::seconds timeout)
            : m_service(std::move(service)), m_timeout { timeout }, m_uuid { GUIDToBluetoothUUID(m_service.Uuid()) }
    {
    }

    [[nodiscard]] BluetoothUUID WindowsBluetoothGattService::GetUUID() const
    {
        return m_uuid;
    }

    [[nodiscard]] GattRegisteredService WindowsBluetoothGattService::GetRegisteredServiceType() const
    {
        return static_cast<GattRegisteredService>(m_uuid.custom);
    }

    std::vector<std::unique_ptr<IBluetoothGattCharacteristic>> &WindowsBluetoothGattService::GetCachedCharacteristics()
    {
        return m_characteristics;
    }

    std::unique_ptr<IBluetoothGattCharacteristic> &WindowsBluetoothGattService::GetCharacteristic(BluetoothUUID uuid)
    {
        static std::unique_ptr<IBluetoothGattCharacteristic> c_nullValue {};

        for (auto &characteristic : m_characteristics) {
            if (characteristic->GetUUID() == uuid) {
                return characteristic;
            }
        }

        return c_nullValue;
    }

    void WindowsBluetoothGattService::FetchCharacteristics()
    {
        WINRT_CALL_BEGIN {
            auto characteristics = WaitWithTimeout(m_service.GetCharacteristicsAsync(BluetoothCacheMode::Uncached), m_timeout);

            if (characteristics.Status() != GattCommunicationStatus::Success) {
                throw BluetoothException("Failed to fetch characteristics");
            }

            m_characteristics.clear();
            for (auto characteristic : characteristics.Characteristics()) {
                m_characteristics.emplace_back(std::make_unique<WindowsBluetoothGattCharacteristic>(std::move(characteristic), m_timeout));
            }
        } WINRT_CALL_END;
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // WindowsBluetoothGattCharacteristic implementation    //
    //                                                      //
    //////////////////////////////////////////////////////////

    WindowsBluetoothGattCharacteristic::WindowsBluetoothGattCharacteristic(GattCharacteristic characteristic, std::chrono::seconds timeout)
            : m_characteristic(std::move(characteristic)), m_timeout { timeout }, m_uuid { GUIDToBluetoothUUID(m_characteristic.Uuid()) }
    {
    }

    [[nodiscard]] BluetoothUUID WindowsBluetoothGattCharacteristic::GetUUID() const
    {
        return m_uuid;
    }

    [[nodiscard]] GattRegisteredCharacteristic WindowsBluetoothGattCharacteristic::GetRegisteredCharacteristicType() const
    {
        return static_cast<GattRegisteredCharacteristic>(m_uuid.custom);
    }

    std::vector<uint8_t> WindowsBluetoothGattCharacteristic::Read()
    {
        WINRT_CALL_BEGIN {
            auto result = WaitWithTimeout(m_characteristic.ReadValueAsync(), m_timeout);

            if (result.Status() != GattCommunicationStatus::Success) {
                throw BluetoothException("Failed to read value");
            }

            auto value = result.Value();
            std::vector<uint8_t> data;
            data.reserve(value.Length());
            data.insert(std::end(data), value.data(), value.data() + value.Length());

            return data;
        } WINRT_CALL_END;
    }

    void WindowsBluetoothGattCharacteristic::Write(const std::vector<uint8_t> &data)
    {
        WINRT_CALL_BEGIN {
            winrt::Windows::Storage::Streams::DataWriter writer;
            writer.WriteBytes(data);

            auto result = WaitWithTimeout(m_characteristic.WriteValueAsync(writer.DetachBuffer()), m_timeout);

            if (result != GattCommunicationStatus::Success) {
                throw BluetoothException("Failed to write value");
            }
        } WINRT_CALL_END;
    }

    size_t WindowsBluetoothGattCharacteristic::Subscribe(std::function<void(std::vector<uint8_t>)> listener)
    {
        WINRT_CALL_BEGIN {
            if (m_subscribers.empty()) {
                auto result = WaitWithTimeout(m_characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify), m_timeout);

                if (result != GattCommunicationStatus::Success) {
                    throw BluetoothException("Failed to write characteristic configuration");
                }
            }

            auto token = m_characteristic.ValueChanged([f = std::move(listener)](const GattCharacteristic &sender, const GattValueChangedEventArgs &args) {
                auto value = args.CharacteristicValue();
                std::vector<uint8_t> vec;
                vec.reserve(value.Length());
                vec.insert(std::end(vec), value.data(), value.data() + value.Length());

                f(std::move(vec));
            });

            m_subscribers.push_back(token);

            return m_subscribers.size() - 1;
        } WINRT_CALL_END;
    }

    void WindowsBluetoothGattCharacteristic::Unsubscribe(size_t id)
    {
        WINRT_CALL_BEGIN {
            m_characteristic.ValueChanged(m_subscribers[id]);
            m_subscribers.erase(m_subscribers.begin() + id);

            if (m_subscribers.empty()) {
                auto result = WaitWithTimeout(m_characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::None), m_timeout);

                if (result != GattCommunicationStatus::Success) {
                    throw BluetoothException("Failed to write characteristic configuration");
                }
            }
        } WINRT_CALL_END;
    }

    void WindowsBluetoothGattCharacteristic::UnsubscribeAll()
    {
        WINRT_CALL_BEGIN {
            for (auto &subscriber : m_subscribers) {
                m_characteristic.ValueChanged(subscriber);
            }

            m_subscribers.clear();

            auto result = WaitWithTimeout(m_characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::None), m_timeout);

            if (result != GattCommunicationStatus::Success) {
                throw BluetoothException("Failed to write characteristic configuration");
            }
        } WINRT_CALL_END;
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // Platform-specific external implementations           //
    //                                                      //
    //////////////////////////////////////////////////////////

    IBluetoothService &GetPlatformLocalBluetoothService()
    {
        static WindowsBluetoothService c_service;
        return c_service;
    }
}
