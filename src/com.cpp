#include <ble_serial/com.hpp>

#include <thread>

namespace BLE_Serial::COM
{
    //////////////////////////////////////////////////////////
    //                                                      //
    // COMException implementation                          //
    //                                                      //
    //////////////////////////////////////////////////////////

    COMException::COMException(std::string message)
            : m_message { std::move(message) }
    {
    }

    const char *COMException::what() const
    {
        return m_message.c_str();
    }

    //////////////////////////////////////////////////////////
    //                                                      //
    // COMPort implementation                               //
    //                                                      //
    //////////////////////////////////////////////////////////

    COMPort::~COMPort()
    {
        UnsubscribeAll();
        Close();
    }

    size_t COMPort::Subscribe(std::function<void(std::vector<uint8_t>)> listener)
    {
        std::unique_lock<std::mutex> lock { m_mutex };
        m_callbacks.emplace_back(std::move(listener));
        m_condition.notify_all();

        if (!m_subscriberThread.joinable()) {
            m_subscriberThread = std::thread([this]() {
                uint8_t buffer[128];

                for (;;) {
                    std::unique_lock<std::mutex> lock { m_mutex };

                    if (m_exiting) {
                        return;
                    }

                    if (m_callbacks.empty()) {
                        m_condition.wait(lock);
                        break;
                    }

                    size_t read = Read(buffer, sizeof(buffer));
                    if (read == 0) {
                        std::this_thread::sleep_for(m_refreshRate);
                        continue;
                    }

                    std::vector<uint8_t> data { buffer, buffer + read };

                    for (auto &callback : m_callbacks) {
                        callback(data);
                    }
                }
            });
        }

        return m_callbacks.size() - 1;
    }

    void COMPort::Unsubscribe(size_t id)
    {
        std::unique_lock<std::mutex> lock { m_mutex };
        m_callbacks.erase(m_callbacks.begin() + id);
        m_condition.notify_all();
    }

    void COMPort::UnsubscribeAll()
    {
        m_exiting = true;
        std::unique_lock<std::mutex> lock { m_mutex };
        m_callbacks.clear();
        m_condition.notify_all();

        if (m_subscriberThread.joinable()) {
            m_subscriberThread.join();
        }
    }

    void COMPort::SetRefreshRate(std::chrono::milliseconds rate) noexcept
    {
        std::unique_lock<std::mutex> lock { m_mutex };
        m_refreshRate = rate;
    }

    std::chrono::milliseconds COMPort::GetRefreshRate() noexcept
    {
        std::unique_lock<std::mutex> lock { m_mutex };
        return m_refreshRate;
    }
}