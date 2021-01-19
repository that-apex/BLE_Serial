#include <iostream>
#include <thread>
#include <csignal>

#include <ble_serial/bluetooth.hpp>
#include <ble_serial/com.hpp>

using namespace BLE_Serial::Bluetooth;
using namespace BLE_Serial::COM;

static std::atomic_bool sigintReceived { false };

void SigintHandler(int)
{
    sigintReceived.store(true);
}

void PrintUsage(const char* name)
{
    std::cout << "BLESerial v0.1.1 by apex_ (GitHub: https://github.com/that-apex/BLE_Serial) \n";
    std::cout << "Correct usage: \n";
    std::cout << "\t" << name << " ls [timeout=5] - Scans for BLE devices for [timeout] seconds and prints the results. \n";
    std::cout << "\t" << name << " query <device_addr> [timeout=5] - Tries to query information from a BLE device with <device_addr> for [timeout] seconds and prints the results. \n";
    std::cout << "\t" << name << " connect <device_addr> <service_id> <characteristic_id> <com_port_number> [timeout=5] [baud=9600] [data=8] [stop=1] [parity=none] [refresh_ms=100]\n";
    std::cout << "\t" << name << " help - Shows this help page \n";

    std::cout << std::flush;
}

int ListDevices(unsigned int timeout)
{
    std::vector<std::unique_ptr<IBluetoothDevice>> output;

    std::cout << "Starting query with timeout of " << timeout << " seconds..." << std::endl;
    IBluetoothService::GetService().ScanDevices(output, std::chrono::seconds(timeout));

    std::cout << "Found " << output.size() << " devices\n";
    size_t i = 1;
    for (auto &device : output) {
        std::cout << "\t" << i++ << ". ";
        std::wcout << device->GetDeviceName();
        std::cout << " [Addr: " << BluetoothAddressToString(device->GetDeviceAddress()) << "]" << "\n";
    }

    std::cout << std::flush;
    return 1;
}

int QueryDevices(BluetoothAddress addr, int timeout)
{
    std::cout << "Connecting ..." << std::endl;

    auto deviceOptional = IBluetoothService::GetService().FindDevice(addr, std::chrono::seconds(timeout));
    if (!deviceOptional) {
        std::cerr << "Device with address: " << BluetoothAddressToString(addr) << " couldn't be found. \n";
        return 1;
    }
    std::cout << "Device found! Connecting ..." << std::endl;

    auto device = std::move(deviceOptional.value());
    auto connection = device->OpenConnection();
    std::cout << "Connected!" << std::endl;

    std::cout << "Device information. \n";
    std::cout << "\tDevice address: " << BluetoothAddressToString(device->GetDeviceAddress()) << "\n";
    std::wcout << "\tDevice name: " << device->GetDeviceName() << "\n";
    std::cout << std::flush;

    std::cout << "\t" << connection->GetServices().size() << " services found: \n";
    for (auto &service : connection->GetServices()) {
        service->FetchCharacteristics();

        std::cout << "\t\t" << IBluetoothService::GetService().UUIDToShortString(service->GetUUID()) << " (Service type: " << GetServiceName(service->GetRegisteredServiceType()).value_or("unknown")
                  << ") with " << service->GetCachedCharacteristics().size() << " characteristics\n";

        for (auto &characteristic : service->GetCachedCharacteristics()) {
            std::cout << "\t\t\t" << IBluetoothService::GetService().UUIDToShortString(characteristic->GetUUID()) << " (Characteristic type: "
                      << GetCharacteristicName(characteristic->GetRegisteredCharacteristicType()).value_or("unknown") << ")" << std::endl;

            if (service->GetRegisteredServiceType() == GattRegisteredService::GenericAccess && characteristic->GetRegisteredCharacteristicType() == GattRegisteredCharacteristic::DeviceName) {
                try {
                    auto value = characteristic->Read();
                    std::string str { reinterpret_cast<char *>(value.data()), value.size() };
                    std::cout << "\t\t\t\tValue: " << str << std::endl;
                } catch (const BluetoothException &ignored) {}
            }
        }

        std::cout << std::flush;
    }

    std::cout << "Disconnecting..." << std::endl;
    connection->Close();
    return 0;
}

int Connect(BluetoothAddress addr, GattRegisteredService serviceId, GattRegisteredCharacteristic characteristicId, unsigned int portNumber, unsigned int timeout, unsigned int baud, unsigned int data, StopBits stopBits, Parity parity, std::chrono::milliseconds refresh)
{
    std::cout << "Searching for device " << BluetoothAddressToString(addr) << " ..." << std::endl;

    auto deviceOptional = IBluetoothService::GetService().FindDevice(addr, std::chrono::seconds(timeout));
    if (!deviceOptional) {
        std::cerr << "Device with address: " << BluetoothAddressToString(addr) << " couldn't be found. \n";
        return 1;
    }

    std::cout << "Device found! Connecting ..." << std::endl;
    auto connection = deviceOptional.value()->OpenConnection();
    std::cout << "Connected!" << std::endl;

    std::cout << "Searching for service 0x" << std::hex << static_cast<uint32_t >(serviceId) << std::dec << "..." << std::endl;
    auto &service = connection->GetService(GetServiceUUID(serviceId));
    if (!service) {
        std::cerr << "Requested service couldn't be found \n";
        return 1;
    }

    std::cout << "Querying characteristics" << std::endl;
    service->FetchCharacteristics();

    std::cout << "Searching for characteristic 0x" << std::hex << static_cast<uint32_t >(serviceId) << std::dec << "..." << std::endl;
    auto &characteristic = service->GetCharacteristic(GetCharacteristicUUID(characteristicId));
    if (!characteristic) {
        std::cerr << "Requested characteristic couldn't be found \n";
        return 1;
    }

    std::cout << "Opening COM" << portNumber << " port..." << std::endl;
    COMPort port { portNumber, baud, data, stopBits, parity };
    port.SetRefreshRate(refresh);

    std::cout << "Subscribing to the characteristic ..." << std::endl;
    characteristic->Subscribe([&](std::vector<uint8_t> data) {
        port.Write(std::move(data));
    });

    std::cout << "Subscribing to the port ..." << std::endl;
    port.Subscribe([&](const std::vector<uint8_t> &data) {
        characteristic->Write(data);
    });

    std::cout << "Working ..." << std::endl;

    signal(SIGINT, SigintHandler);

    while (!sigintReceived.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Exiting ..." << std::endl;

    port.UnsubscribeAll();
    port.Close();

    characteristic->UnsubscribeAll();
    connection->Close();

    std::cout << "Good bye!" << std::endl;
    return 0;
}

struct ParamHelper
{
    int argc;
    char **argv;

    std::string GetStringOrDefault(size_t index, const char *def) const
    {
        if (argc <= index) {
            return std::string { def };
        }

        return std::string { argv[index] };
    }

    template<typename T, typename Conv>
    T GetOrDefault(size_t index, const char *def, Conv &&converter) const
    {
        return converter(GetStringOrDefault(index, def));
    }
};

int StringToInt(const std::string &str)
{
    return std::stoi(str);
}

StopBits StopBitsFromString(const std::string &str)
{
    if (str == "1") {
        return STOP_BITS_ONE;
    } else if (str == "1.5") {
        return STOP_BITS_ONE_AND_HALF;
    } else if (str == "2") {
        return STOP_BITS_TWO;
    } else {
        throw std::invalid_argument("Valid arguments for StopBits are: 1; 1.5; 2");
    }
}

Parity ParityFromString(const std::string &str)
{
    if (str == "none") {
        return PARITY_BITS_NONE;
    } else if (str == "odd") {
        return PARITY_BITS_ODD;
    } else if (str == "even") {
        return PARITY_BITS_EVEN;
    } else if (str == "mark") {
        return PARITY_BITS_MARK;
    } else if (str == "space") {
        return PARITY_BITS_SPACE;
    } else {
        throw std::invalid_argument("Valid arguments for Parity are: none, odd, even, mark, space");
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string action { argv[1] };
    IBluetoothService::GetService().Initialize();

    // @formatter:off
    try {
        ParamHelper args { argc, argv };

        if (action == "ls") {
            return ListDevices(args.GetOrDefault<int>(2, "5", &StringToInt));
        } else if (action == "query" && argc >= 3) {
            return QueryDevices(
                    args.GetOrDefault<BluetoothAddress>(2, "", &BluetoothAddressFromString),
                    args.GetOrDefault<int>(3, "5", &StringToInt)
            );
        } else if (action == "help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (action == "connect" && argc >= 4) {
            return Connect(
                    args.GetOrDefault<BluetoothAddress>(2, "", &BluetoothAddressFromString),
                    args.GetOrDefault<GattRegisteredService>(3, "", [](const std::string& str) { return static_cast<GattRegisteredService>(std::stoi(str, nullptr, 16)); }),
                    args.GetOrDefault<GattRegisteredCharacteristic>(4, "", [](const std::string& str) { return static_cast<GattRegisteredCharacteristic>(std::stoi(str, nullptr, 16)); }),
                    args.GetOrDefault<int>(5, "", &StringToInt),
                    args.GetOrDefault<int>(6, "5", &StringToInt),
                    args.GetOrDefault<int>(7, "9600", &StringToInt),
                    args.GetOrDefault<int>(8, "8", &StringToInt),
                    args.GetOrDefault<StopBits>(9, "1", &StopBitsFromString),
                    args.GetOrDefault<Parity>(10, "none", &ParityFromString),
                    args.GetOrDefault<std::chrono::milliseconds>(11, "100", [](const std::string& str) { return std::chrono::milliseconds(StringToInt(str)); })
            );
        } else {
            PrintUsage(argv[0]);
            return 1;
        }
    } catch (const std::invalid_argument &e) {
        std::cerr << "Invalid argument: " << e.what();
        return 1;
    } catch (const BluetoothException &e) {
        std::cerr << "Bluetooth error: " << e.what();
        return 1;
    } catch (const COMException &e) {
        std::cerr << "COM error: " << e.what();
        return 1;
    }
    // @formatter:on
}
