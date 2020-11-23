#ifndef BLE_SERIAL_INCLUDE_COM_HPP_
#define BLE_SERIAL_INCLUDE_COM_HPP_

#include <string>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @brief Serial port API
 */
namespace BLE_Serial::COM
{
    /**
     * @brief General exception for all kinds of COM port errors.
     */
    class COMException : std::exception
    {
    public:
        /**
         * @brief Construct new @link COMException @endlink
         *
         * @param message error details
         */
        explicit COMException(std::string message);

        /**
         * @return error details
         */
        [[nodiscard]]  const char *what() const override;

    private:
        std::string m_message;
    };

    /**
     * @brief Represents a number of parity bits in a serial connection
     */
    enum Parity
    {
        /**
         * No parity bits
         */
        PARITY_BITS_NONE,

        /**
         * Odd parity bit
         */
        PARITY_BITS_ODD,

        /**
         * Even parity bit
         */
        PARITY_BITS_EVEN,

        /**
         * Mark parity bit
         */
        PARITY_BITS_MARK,

        /**
         * Space parity bit
         */
        PARITY_BITS_SPACE
    };

    /**
     * @brief Represents a number of stop bits in a serial connection
     */
    enum StopBits
    {
        /**
         * One stop bit
         */
        STOP_BITS_ONE,

        /**
         * One and a half stop bits
         */
        STOP_BITS_ONE_AND_HALF,

        /**
         * Two stop bits
         */
        STOP_BITS_TWO
    };

    /**
     * @brief Represents a serial connection over a COM port.
     */
    class COMPort
    {
    public:
        /**
         * @brief Constructs a new COMPort and opens a serial connection.
         *
         * @param number COM port number (i.e. por 1 is COM1 in Windows)
         * @param baud bits per second
         * @param data number of data bits
         * @param stopBits number of stop bits
         * @param parity parity bit settings
         *
         * @throws COMException when the port initialization fails
         */
        COMPort(unsigned int number, unsigned int baud, unsigned int data = 8, StopBits stopBits = STOP_BITS_ONE, Parity parity = PARITY_BITS_NONE);

        /**
         * Destructs the port, closes the serial connection and stops all subscriber threads.
         */
        ~COMPort();

        /**
         * @brief Writes data to this serial port.
         *
         * @param data data to be written
         *
         * @return how many bytes were actually written
         */
        size_t Write(std::vector<uint8_t> data);

        /**
         * @brief Reads data from this serial port.
         *
         * @param buffer buffer where the data will be stored
         * @param size size of the input buffer
         *
         * @return how many bytes were actually read
         */
        size_t Read(uint8_t *buffer, size_t size);

        /**
         * @brief Subscribes to new data coming to this serial port.
         *
         * @param listener listener to be called every time there is new data in the serial port
         *
         * @return id of the listener, used for the @link Unsubscribe @endlink function.
         */
        size_t Subscribe(std::function<void(std::vector<uint8_t>)> listener);

        /**
         * Unsubscribes a listener previously registered with @link Subscribe @endlink
         *
         * @param id id of the listener
         */
        void Unsubscribe(size_t id);

        /**
         * @brief Unsubscribes all listeners previously registered with @link Subscribe @endlink
         */
        void UnsubscribeAll();

        /**
         * @brief Closes the port.
         */
        void Close();

        /**
         * Sets the refresh rate for all subscriber threads.
         *
         * @param rate refresh rate
         */
        void SetRefreshRate(std::chrono::milliseconds rate) noexcept;

        /**
         * Gets the refresh rate for all subscriber threads.
         *
         * @return refresh rate
         */
        std::chrono::milliseconds GetRefreshRate() noexcept;

    private:
        void *m_handle;

        std::thread m_subscriberThread {};
        std::chrono::milliseconds m_refreshRate { 100 };
        std::mutex m_mutex {};
        std::condition_variable m_condition {};
        bool m_exiting = false;
        std::vector<std::function<void(std::vector<uint8_t>)>> m_callbacks {};
    };

}

#endif // BLE_SERIAL_INCLUDE_COM_HPP_
