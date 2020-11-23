#include <ble_serial/com.hpp>

#include <windows.h>
#include <string>

namespace BLE_Serial::COM
{
    //////////////////////////////////////////////////////////
    //                                                      //
    // COMPort implementation                               //
    //                                                      //
    //////////////////////////////////////////////////////////

    COMPort::COMPort(unsigned int number, unsigned int baud, unsigned int data, StopBits stopBits, Parity parity)
            : m_handle { nullptr }
    {
        std::string port = "COM";
        port += std::to_string(number);

        m_handle = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (m_handle == INVALID_HANDLE_VALUE) {
            throw COMException("Failed to open port " + port);
        }

        DCB dcb;
        SecureZeroMemory(&dcb, sizeof(DCB));
        dcb.DCBlength = sizeof(DCB);

        if (!GetCommState(m_handle, &dcb)) {
            throw COMException("GetCommState failed with error " + std::to_string(GetLastError()));
        }

        dcb.BaudRate = baud;
        dcb.ByteSize = data;
        switch (stopBits) {
            case STOP_BITS_ONE:
                dcb.StopBits = ONESTOPBIT;
                break;
            case STOP_BITS_ONE_AND_HALF:
                dcb.StopBits = ONE5STOPBITS;
                break;
            default:
                dcb.StopBits = TWOSTOPBITS;
                break;
        }

        switch (parity) {
            case PARITY_BITS_NONE:
                dcb.Parity = NOPARITY;
                break;
            case PARITY_BITS_ODD:
                dcb.Parity = ODDPARITY;
                break;
            case PARITY_BITS_EVEN:
                dcb.Parity = EVENPARITY;
                break;
            case PARITY_BITS_MARK:
                dcb.Parity = MARKPARITY;
                break;
            case PARITY_BITS_SPACE:
                dcb.Parity = SPACEPARITY;
                break;
        }

        if (!SetCommState(m_handle, &dcb)) {
            throw COMException("SetCommState failed with error " + std::to_string(GetLastError()));
        }

        COMMTIMEOUTS timeouts;

        timeouts.ReadIntervalTimeout = 1;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        timeouts.ReadTotalTimeoutConstant = 1;
        timeouts.WriteTotalTimeoutMultiplier = 1;
        timeouts.WriteTotalTimeoutConstant = 1;

        if (!SetCommTimeouts(m_handle, &timeouts)) {
            throw COMException("SetCommTimeouts failed with error " + std::to_string(GetLastError()));
        }
    }

    size_t COMPort::Write(std::vector<uint8_t> data)
    {
        DWORD written;
        WriteFile(m_handle, data.data(), data.size(), &written, nullptr);

        return written;
    }

    size_t COMPort::Read(uint8_t *buffer, size_t size)
    {
        DWORD read;
        ReadFile(m_handle, buffer, size, &read, nullptr);

        return read;
    }

    void COMPort::Close()
    {
        if (m_handle != nullptr) {
            CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }
}