# BLE_Serial
Application for connecting Bluetooth LE devices to COM ports.

# Supported platforms
- Windows

# Development requierements
- CMake version 3.16 or newer.
- MSVC compiler (may work on other compilers, untested)
- UWP (on Windows)

# Commands

### ble_serial ls \[timeout\]
#### Description
Scans for BLE

### Arguments

- `timeout` - how long should the scan be running for (in seconds) \[Default: 5 seconds\]
devices for `timeout` seconds and prints the results.

### ble_serial query <device_addr> \[timeout=5\]
#### Description

Connects to a BLE device with the given address and queries it for all its services and characteristics.

### Arguments

- `device_addr` - address of the device that we are trying to connect to (can be obtained with `ble_serial ls`)
- `timeout` - maximum time for estabilishing a connection with the device (in seconds) \[Default: 5 seconds\]


### ble_serial connect <device_addr> <service_id> <characteristic_id> <com_port_number> \[timeout\] \[baud\] \[data\] \[stop\] \[parity\] \[refresh_ms\]
#### Description
Connects to a BLE device and subscribes to the characteristic with the given `characteristic_id` and binds it to a COM port and makes a bidirectional tunnel.

Any data written to the bound COM port will be written to the bound BLE characteristic.

After the bound characteristic changes the data will be written to the bound COM port.

### Arguments

- `device_addr` - address of the device that we are trying to connect to (can be obtained with `ble_serial ls`)
- `service_id` - id of the service to be bound to the COM port
- `characteristic_id` - id of the service to be bound to the COM port
- `com_port_number` - number of a com port that will be used for binding
- `timeout` - maximum time for estabilishing a connection with the device (in seconds) [Default: 5 seconds]
- `baud`, `data`, `stop`, `parity` - COM port settings (baud rate, data bits, stop bits, parity bits) [Default: 8-N-1]
- `refresh_ms` - every how many milliseconds should the COM port be refreshed [Default: 100 ms]

# Library
The BLE_Serial can be used also as a C++ library for interfacing with BLE devices and COM ports.

### How to use
This project uses CMake. You can use git submodules to directly add the project's source tree to your project.

Then you can link against `BLE_Serial_Lib` target in your project.

### Example CMake file
```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject CXX)

add_subdirectory("path_to_ble_serial_directory")

add_executable(MyExecutable
        src/main.cpp
)

target_link_libraries(BLE_Serial
        PUBLIC
            BLE_Serial_Lib
)
```

### Example C++ aplication
For an example application use you can see [the BLE_Serial app source code](https://github.com/that-apex/BLE_Serial/blob/master/src/main.cpp)

### Documentation
The library documentation can be found [here](https://that-apex.github.io/BLE_Serial)
