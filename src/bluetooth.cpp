#include <ble_serial/bluetooth.hpp>

#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace BLE_Serial::Bluetooth
{
    /**
     * Helper function for retrieving platform-specific IBluetoothService
     */
    extern IBluetoothService &GetPlatformLocalBluetoothService();

    namespace
    {
        std::unordered_map<GattRegisteredService, std::string> g_serviceNameCache {}; // NOLINT(cert-err58-cpp)
        std::unordered_map<GattRegisteredCharacteristic, std::string> g_characteristicNameCache; // NOLINT(cert-err58-cpp)

        /**
         * Initializes the GATT names cache
         */
        void InitializeCache()
        {
            static bool c_cacheInitialized = false;

            if (c_cacheInitialized) {
                return;
            }

            g_serviceNameCache.reserve(50);
            g_characteristicNameCache.reserve(300);

            #define GATT_SERVICE(id, name) g_serviceNameCache[static_cast<GattRegisteredService>(id)] = name
            #define GATT_CHARACTERISTIC(id, name) g_characteristicNameCache[static_cast<GattRegisteredCharacteristic>(id)] = name

            #include "gatt_db.cpp"

            #undef GATT_SERVICE
            #undef GATT_CHARACTERISTIC

            c_cacheInitialized = true;
        }

        /**
         * Helper for converting short UUIDs to long UUIds
         */
        BluetoothUUID GetBluetoothUUID(uint32_t uuid)
        {
            static BluetoothUUID c_genericUUID = IBluetoothService::GetService().UUIDFromString("00000000-0000-1000-8000-00805F9B34FB");

            BluetoothUUID result = c_genericUUID;
            result.custom = uuid;
            return result;
        }
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // Public functions                                     //
    //                                                      //
    //////////////////////////////////////////////////////////
    bool operator==(const BluetoothUUID &lhs, const BluetoothUUID &rhs)
    {
        return memcmp(&lhs, &rhs, sizeof(BluetoothUUID)) == 0;
    }

    std::string BluetoothAddressToString(BluetoothAddress address)
    {
        union
        {
            uint8_t bytes[8];
            uint64_t value;
        } helper { .value = address };

        char buffer[20];
        sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", helper.bytes[5], helper.bytes[4], helper.bytes[3], helper.bytes[2], helper.bytes[1], helper.bytes[0]);

        return std::string { buffer };
    }

    BluetoothAddress BluetoothAddressFromString(std::string address)
    {
        address.erase(std::remove(std::begin(address), std::end(address), ':'), std::end(address));

        return static_cast<BluetoothAddress>(std::stoull(address, nullptr, 16));
    }

    std::optional<std::string> GetServiceName(GattRegisteredService service)
    {
        InitializeCache();

        auto it = g_serviceNameCache.find(service);
        return it == g_serviceNameCache.end() ? std::nullopt : std::make_optional<std::string>(it->second);
    }

    std::optional<std::string> GetCharacteristicName(GattRegisteredCharacteristic characteristic)
    {
        InitializeCache();

        auto it = g_characteristicNameCache.find(characteristic);
        return it == g_characteristicNameCache.end() ? std::nullopt : std::make_optional<std::string>(it->second);
    }

    BluetoothUUID GetServiceUUID(GattRegisteredService service)
    {
        return GetBluetoothUUID(static_cast<uint32_t>(service));
    }

    BluetoothUUID GetCharacteristicUUID(GattRegisteredCharacteristic characteristic)
    {
        return GetBluetoothUUID(static_cast<uint32_t>(characteristic));
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // BluetoothException implementation                    //
    //                                                      //
    //////////////////////////////////////////////////////////

    BluetoothException::BluetoothException(std::string message)
            : m_message { std::move(message) }
    {
    }

    const char *BluetoothException::what() const
    {
        return m_message.c_str();
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // IBluetoothService implementation                     //
    //                                                      //
    //////////////////////////////////////////////////////////

    IBluetoothService &IBluetoothService::GetService()
    {
        return GetPlatformLocalBluetoothService();
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // IBluetoothDevice implementation                      //
    //                                                      //
    //////////////////////////////////////////////////////////

    std::shared_ptr<IBluetoothConnection> IBluetoothDevice::OpenConnection()
    {
        return OpenConnection(DefaultTimeout);
    }
}
