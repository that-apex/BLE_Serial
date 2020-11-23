#ifndef BLE_SERIAL_INCLUDE_BLUETOOTH_HPP_
#define BLE_SERIAL_INCLUDE_BLUETOOTH_HPP_

#include <exception>
#include <chrono>
#include <memory>
#include <functional>
#include <string>
#include <optional>
#include <vector>

/**
 * @brief Bluetooth LE API
 */
namespace BLE_Serial::Bluetooth
{
    // Forward declarations
    class IBluetoothDevice;
    class IBluetoothConnection;
    class IBluetoothGattService;
    class IBluetoothGattCharacteristic;
    enum class GattRegisteredService : uint32_t;
    enum class GattRegisteredCharacteristic : uint32_t;

    /**
     * The default timeout for all Bluetooth operations.
     */
    constexpr std::chrono::seconds DefaultTimeout = std::chrono::seconds(1);

    /**
     * @brief General exception for all kinds of Bluetooth errors.
     */
    class BluetoothException : std::exception
    {
    public:
        /**
         * @brief Construct new @link BluetoothException @endlink
         *
         * @param message error details
         */
        explicit BluetoothException(std::string message);

        /**
         * @return error details
         */
        [[nodiscard]]  const char *what() const override;

    private:
        std::string m_message;
    };

    /**
     * @brief Represents an address of a Bluetooth device.
     */
    using BluetoothAddress = uint64_t;

    /**
     * @brief Represents a bluetooth UUID.
     */
    struct BluetoothUUID
    {
        uint32_t custom;  ///< @private
        uint16_t part2;   ///< @private
        uint16_t part3;   ///< @private
        uint8_t part4[8]; ///< @private
    };

    /**
     * @brief Compare two @link BluetoothUUID BluetoothUUIDs @endlink
     *
     * @param lhs first UUID to compare
     * @param rhs second UUID to compare
     *
     * @return true only if  both of the UUIDs are identical.
     */
    bool operator==(const BluetoothUUID &lhs, const BluetoothUUID &rhs);

    /**
     * @brief Converts the @link BluetoothAddress @endlink into a human-readable string.
     *
     * The formatted string will have the following format XX:XX:XX:XX:XX where X is a hexadecimal digit.
     *
     * @param address address to convert
     *
     * @return converted string
     */
    std::string BluetoothAddressToString(BluetoothAddress address);

    /**
     * @brief Converts a string into a @link BluetoothAddress @endlink.
     *
     * The formatted string must match the following format XX:XX:XX:XX:XX where X is a hexadecimal digit.
     *
     * @param address string to convert
     *
     * @return address
     */
    BluetoothAddress BluetoothAddressFromString(std::string address);

    /**
     * @brief Retrieves a name of a @link GattRegisteredService @endlink.
     *
     * @param service service
     *
     * @return name of the service or an empty optional if the service is not registered .
     */
    std::optional<std::string> GetServiceName(GattRegisteredService service);

    /**
     * @brief Retrieves a name of a @link GattRegisteredCharacteristic @endlink.
     *
     * @param characteristic characteristic
     *
     * @return name of the characteristic or an empty optional if the characteristic is not registered .
     */
    std::optional<std::string> GetCharacteristicName(GattRegisteredCharacteristic characteristic);

    /**
     * @brief Gets a full @link BluetoothUUID @endlink for a @link GattRegisteredService @endlink.
     *
     * @param service service, may be either a registered or an unregistered GATT service id
     *
     * @return the full UUID
     */
    BluetoothUUID GetServiceUUID(GattRegisteredService service);

    /**
     * @brief Gets a full @link BluetoothUUID @endlink for a @link GattRegisteredCharacteristic @endlink.
     *
     * @param characteristic characteristic, may be either a registered or an unregistered GATT characteristic id
     *
     * @return the full UUID
     */
    BluetoothUUID GetCharacteristicUUID(GattRegisteredCharacteristic characteristic);

    /**
     * @brief Represents an intermediate service used to communicate with the native OS Bluetooth API.
     */
    class IBluetoothService
    {
    public:
        /**
         * @brief Initializes the service.
         *
         * Must be called before any other methods are used.
         */
        virtual void Initialize() = 0;

        /**
         * @brief Converts a string into a @link BluetoothUUID @endlink.
         * The string must match the following format XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX where X is a hexadecimal digit.
         *
         * @param string string to convert
         * @return converted UUID
         *
         * @throw BluetoothException when the string is incorrectly formatted.
         */
        virtual BluetoothUUID UUIDFromString(std::string_view string) = 0;

        /**
         * @brief Converts a @link BluetoothUUID @endlink into a string.
         *
         * The string will have the following format XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX where X is a hexadecimal digit.
         *
         * @param uuid to convert
         * @return converted string
         */
        virtual std::string UUIDToString(BluetoothUUID uuid) = 0;

        /**
         * @brief Converts a @link BluetoothUUID @endlink into a shortened string.
         *
         * The string will have the following format XXXXXXXX where X is a hexadecimal digit.
         * Only the first 4 bytes are printed because the rest is static according to the specification and unnecessary.
         *
         * @param uuid to convert
         * @return converted string
         */
        virtual std::string UUIDToShortString(BluetoothUUID uuid) = 0;

        /**
         * @brief Scans for all BLE devices for the given amount of seconds.
         *
         * This function in blocking.
         *
         * @param output vector that will be populated with devices found during the scanning period
         * @param timeout for how many seconds should the scan be running
         */
        virtual void ScanDevices(std::vector<std::unique_ptr<IBluetoothDevice>> &output, std::chrono::seconds timeout) = 0;

        /**
         * @brief Scans to find a BLE device that matches the following address.
         *
         * This function in blocking.
         *
         * @param address address to match
         * @param timelimit for how many seconds at most should the scan be running before giving up
         *
         * @return the device or an empty optional if the device was not found in the given timelimit
         */
        virtual std::optional<std::unique_ptr<IBluetoothDevice>> FindDevice(BluetoothAddress address, std::chrono::seconds timelimit) = 0;

    public:
        /**
         * @brief Gets the @link IBluetoothService @endlink for the current platform.
         *
         * @return the implementation
         */
        static IBluetoothService &GetService();

    protected:
        IBluetoothService() = default;

    };

    /**
     * @brief Represents a BLE device.
     */
    class IBluetoothDevice
    {
    public:
        /**
         * @brief Returns the device's @link BluetoothAddress @endlink
         *
         * @return the device's @link BluetoothAddress @endlink
         */
        [[nodiscard]] virtual BluetoothAddress GetDeviceAddress() const noexcept = 0;

        /**
         * @brief Returns the device's name
         *
         * @return the device's name
         */
        [[nodiscard]] virtual const std::wstring &GetDeviceName() const noexcept = 0;

        /**
         * @brief Gets an open connection.
         *
         * @return an open connection or an empty optional if the device is not connected
         */
        [[nodiscard]] virtual std::optional<std::shared_ptr<IBluetoothConnection>> GetOpenConnection() const noexcept = 0;

        /**
         * @brief Opens a new connection with the default timeout.
         *
         * Calling the function is equivalent to calling @code OpenConnection(DefaultTimeout) @endcode
         *
         * @return the newly opened connection.
         *
         * @throw BluetoothException when the connection fails
         */
        [[nodiscard]] virtual std::shared_ptr<IBluetoothConnection> OpenConnection();

        /**
         * @brief Opens a new connection with the given timeout.
         *
         * @param timeout timeout for the connection, this timeout will also be used for subsequent calls to @link IBluetoothConnection IBluetoothConnection's@endlink methods.
         * @return the newly opened connection.
         *
         * @throw BluetoothException when the connection fails
         */
        [[nodiscard]] virtual std::shared_ptr<IBluetoothConnection> OpenConnection(std::chrono::seconds timeout) = 0;

    protected:
        IBluetoothDevice() = default;
    };

    /**
     * @brief Represents a connection to a @link IBluetoothDevice @endlink
     */
    class IBluetoothConnection
    {
    public:
        /**
         * @brief Checks whether or not the connection is still open.
         *
         * @return whether or not the connection is still open.
         */
        [[nodiscard]] virtual bool IsOpen() const noexcept = 0;

        /**
         * @brief Closes the connection.
         *
         * @throw BluetoothException when the operation fails
         */
        virtual void Close() = 0;

        /**
         * @brief Gets all @link IBluetoothGattService IBluetoothGattServices @endlink registered to this device.
         *
         * @return all registered services.
         */
        [[nodiscard]] virtual std::vector<std::unique_ptr<IBluetoothGattService>> &GetServices() = 0;

        /**
         * @brief Gets an @link IBluetoothGattService @endlink registered to this device that matches the given UUID.
         *
         * @return service that matches the given UUID or an empty unique_ptr if no such service is found.
         */
        [[nodiscard]] virtual std::unique_ptr<IBluetoothGattService> &GetService(BluetoothUUID uuid) = 0;

    protected:
        IBluetoothConnection() = default;
    };

    /**
     * @brief Represents a GATT service.
     */
    class IBluetoothGattService
    {
    public:
        /**
         * @brief Returns the UUID of this service.
         *
         * @return the UUID of this service.
         */
        [[nodiscard]] virtual BluetoothUUID GetUUID() const = 0;

        /**
         * @brief Returns the @link GattRegisteredService @endlink of this service.
         *
         * @return the @link GattRegisteredService @endlink of this service.
         */
        [[nodiscard]] virtual GattRegisteredService GetRegisteredServiceType() const = 0;

        /**
         * @brief Gets all @link IBluetoothGattCharacteristic IBluetoothGattCharacteristics @endlink registered to this device.
         *
         * @link FetchCharacteristics @endlink must be called before using this function.
         *
         * @return all registered characteristics.
         */
        [[nodiscard]] virtual std::vector<std::unique_ptr<IBluetoothGattCharacteristic>> &GetCachedCharacteristics() = 0;

        /**
         * @brief Gets an @link IBluetoothGattCharacteristic @endlink registered to this service that matches the given UUID.
         *
         * @link FetchCharacteristics @endlink must be called before using this function.
         *
         * @return characteristic that matches the given UUID or an empty unique_ptr if no such characteristic is found.
         */
        [[nodiscard]] virtual std::unique_ptr<IBluetoothGattCharacteristic> &GetCharacteristic(BluetoothUUID uuid) = 0;

        /**
         * @brief Retrieves all the characteristics from the remote device.
         *
         * The characteristics can be then iterated via @link GetCachedCharacteristics @endlink
         *
         * @throw BluetoothException when the operation fails
         */
        virtual void FetchCharacteristics() = 0;

    protected:
        IBluetoothGattService() = default;
    };

    /**
     * @brief Represents a GATT characteristic.
     */
    class IBluetoothGattCharacteristic
    {
    public:

        /**
         * @brief Returns the UUID of this characteristic.
         *
         * @return the UUID of this characteristic.
         */
        [[nodiscard]] virtual BluetoothUUID GetUUID() const = 0;

        /**
         * @brief Returns the @link GattRegisteredCharacteristic @endlink of this characteristic.
         *
         * @return the @link GattRegisteredCharacteristic @endlink of this characteristic.
         */
        [[nodiscard]] virtual GattRegisteredCharacteristic GetRegisteredCharacteristicType() const = 0;

        /**
         * @brief Reads data from this characteristic.
         *
         * @return vector containing the read data
         *
         * @throw BluetoothException when the operation fails
         */
        [[nodiscard]] virtual std::vector<uint8_t> Read() = 0;

        /**
         * @brief Writes data to this characteristic.
         *
         * @param data vector containing the data to be written
         *
         * @throw BluetoothException when the operation fails
         */
        virtual void Write(const std::vector<uint8_t> &data) = 0;

        /**
         * @brief Subscribes to all changes of this characteristic's data.
         *
         * @param listener listener to be called every time the characteristic's data changes
         *
         * @return id of the listener, used for the @link Unsubscribe @endlink function.
         */
        virtual size_t Subscribe(std::function<void(std::vector<uint8_t>)> listener) = 0;

        /**
         * @brief Unsubscribes a listener previously registered with @link Subscribe @endlink
         *
         * @param id id of the listener
         */
        virtual void Unsubscribe(size_t id) = 0;

        /**
         * @brief Unsubscribes all listeners previously registered with @link Subscribe @endlink
         */
        virtual void UnsubscribeAll() = 0;

    protected:
        IBluetoothGattCharacteristic() = default;
    };

    /**
     * @brief Represents all the registered GATT service types.
     */
    enum class GattRegisteredService : uint32_t
    {
        GenericAccess = 0x1800,
        GenericAttribute = 0x1801,
        ImmediateAlert = 0x1802,
        LinkLoss = 0x1803,
        TxPower = 0x1804,
        CurrentTime = 0x1805,
        ReferenceTimeUpdate = 0x1806,
        NextDSTChange = 0x1807,
        Glucose = 0x1808,
        HealthThermometer = 0x1809,
        DeviceInformation = 0x180A,
        HeartRate = 0x180D,
        PhoneAlertStatus = 0x180E,
        Battery = 0x180F,
        BloodPressure = 0x1810,
        AlertNotification = 0x1811,
        HumanInterfaceDevice = 0x1812,
        ScanParameters = 0x1813,
        RunningSpeedAndCadence = 0x1814,
        AutomationIO = 0x1815,
        CyclingSpeedAndCadence = 0x1816,
        CyclingPower = 0x1818,
        LocationAndNavigation = 0x1819,
        EnvironmentalSensing = 0x181A,
        BodyComposition = 0x181B,
        UserData = 0x181C,
        WeightScale = 0x181D,
        BondManagement = 0x181E,
        ContinuousGlucoseMonitoring = 0x181F,
        InternetProtocolSupport = 0x1820,
        IndoorPositioning = 0x1821,
        PulseOximeter = 0x1822,
        HTTPProxy = 0x1823,
        TransportDiscovery = 0x1824,
        ObjectTransfer = 0x1825,
        FitnessMachine = 0x1826,
        MeshProvisioning = 0x1827,
        MeshProxy = 0x1828,
        ReconnectionConfiguration = 0x1829,
        InsulinDelivery = 0x183A,
        BinarySensor = 0x183B,
        EmergencyConfiguration = 0x183C,
        HM10 = 0xFFE0
    };

    /**
     * @brief Represents all the registered GATT characteristic types.
     */
    enum class GattRegisteredCharacteristic : uint32_t
    {
        DeviceName = 0x2A00,
        Appearance = 0x2A01,
        PeripheralPrivacyFlag = 0x2A02,
        ReconnectionAddress = 0x2A03,
        PeripheralPreferredConnectionParameters = 0x2A04,
        ServiceChanged = 0x2A05,
        AlertLevel = 0x2A06,
        TxPowerLevel = 0x2A07,
        DateTime = 0x2A08,
        DayOfWeek = 0x2A09,
        DayDateTime = 0x2A0A,
        ExactTime256 = 0x2A0C,
        DstOffset = 0x2A0D,
        TimeZone = 0x2A0E,
        LocalTimeInformation = 0x2A0F,
        TimeWithDst = 0x2A11,
        TimeAccuracy = 0x2A12,
        TimeSource = 0x2A13,
        ReferenceTimeInformation = 0x2A14,
        TimeUpdateControlPoint = 0x2A16,
        TimeUpdateState = 0x2A17,
        GlucoseMeasurement = 0x2A18,
        BatteryLevel = 0x2A19,
        TemperatureMeasurement = 0x2A1C,
        TemperatureType = 0x2A1D,
        IntermediateTemperature = 0x2A1E,
        MeasurementInterval = 0x2A21,
        BootKeyboardInputReport = 0x2A22,
        SystemId = 0x2A23,
        ModelNumberString = 0x2A24,
        SerialNumberString = 0x2A25,
        FirmwareRevisionString = 0x2A26,
        HardwareRevisionString = 0x2A27,
        SoftwareRevisionString = 0x2A28,
        ManufacturerNameString = 0x2A29,
        Ieee11073_20601RegulatoryCertificationDataList = 0x2A2A,
        CurrentTime = 0x2A2B,
        ScanRefresh = 0x2A31,
        BootKeyboardOutputReport = 0x2A32,
        BootMouseInputReport = 0x2A33,
        GlucoseMeasurementContext = 0x2A34,
        BloodPressureMeasurement = 0x2A35,
        IntermediateCuffPressure = 0x2A36,
        HeartRateMeasurement = 0x2A37,
        BodySensorLocation = 0x2A38,
        HeartRateControlPoint = 0x2A39,
        AlertStatus = 0x2A3F,
        RingerControlPoint = 0x2A40,
        RingerSetting = 0x2A41,
        AlertCategoryIdBitMask = 0x2A42,
        AlertCategoryId = 0x2A43,
        AlertNotificationControlPoint = 0x2A44,
        UnreadAlertStatus = 0x2A45,
        NewAlert = 0x2A46,
        SupportedNewAlertCategory = 0x2A47,
        SupportedUnreadAlertCategory = 0x2A48,
        BloodPressureFeature = 0x2A49,
        HidInformation = 0x2A4A,
        ReportMap = 0x2A4B,
        HidControlPoint = 0x2A4C,
        Report = 0x2A4D,
        ProtocolMode = 0x2A4E,
        ScanIntervalWindow = 0x2A4F,
        PnPId = 0x2A50,
        GlucoseFeature = 0x2A51,
        RecordAccessControlPoint = 0x2A52,
        RscMeasurement = 0x2A53,
        RscFeature = 0x2A54,
        ScControlPoint = 0x2A55,
        Aggregate = 0x2A5A,
        CscMeasurement = 0x2A5B,
        CscFeature = 0x2A5C,
        SensorLocation = 0x2A5D,
        PlxSpotCheckMeasurement = 0x2A5E,
        PlxContinuousMeasurement = 0x2A5F,
        PlxFeatures = 0x2A60,
        CyclingPowerMeasurement = 0x2A63,
        CyclingPowerVector = 0x2A64,
        CyclingPowerFeature = 0x2A65,
        CyclingPowerControlPoint = 0x2A66,
        LocationAndSpeed = 0x2A67,
        Navigation = 0x2A68,
        PositionQuality = 0x2A69,
        LnFeature = 0x2A6A,
        LnControlPoint = 0x2A6B,
        Elevation = 0x2A6C,
        Pressure = 0x2A6D,
        Temperature = 0x2A6E,
        Humidity = 0x2A6F,
        TrueWindSpeed = 0x2A70,
        TrueWindDirection = 0x2A71,
        ApparentWindSpeed = 0x2A72,
        ApparentWindDirection = 0x2A73,
        GustFactor = 0x2A74,
        PollenConcentration = 0x2A75,
        UvIndex = 0x2A76,
        Irradiance = 0x2A77,
        Rainfall = 0x2A78,
        WindChill = 0x2A79,
        HeatIndex = 0x2A7A,
        DewPoint = 0x2A7B,
        DescriptorValueChanged = 0x2A7D,
        AerobicHeartRateLowerLimit = 0x2A7E,
        AerobicThreshold = 0x2A7F,
        Age = 0x2A80,
        AnaerobicHeartRateLowerLimit = 0x2A81,
        AnaerobicHeartRateUpperLimit = 0x2A82,
        AnaerobicThreshold = 0x2A83,
        AerobicHeartRateUpperLimit = 0x2A84,
        DateOfBirth = 0x2A85,
        DateOfThresholdAssessment = 0x2A86,
        EmailAddress = 0x2A87,
        FatBurnHeartRateLowerLimit = 0x2A88,
        FatBurnHeartRateUpperLimit = 0x2A89,
        FirstName = 0x2A8A,
        FiveZoneHeartRateLimits = 0x2A8B,
        Gender = 0x2A8C,
        HeartRateMax = 0x2A8D,
        Height = 0x2A8E,
        HipCircumference = 0x2A8F,
        LastName = 0x2A90,
        MaximumRecommendedHeartRate = 0x2A91,
        RestingHeartRate = 0x2A92,
        SportTypeForAerobicAndAnaerobicThresholds = 0x2A93,
        ThreeZoneHeartRateLimits = 0x2A94,
        TwoZoneHeartRateLimits = 0x2A95,
        Vo2Max = 0x2A96,
        WaistCircumference = 0x2A97,
        Weight = 0x2A98,
        DatabaseChangeIncrement = 0x2A99,
        UserIndex = 0x2A9A,
        BodyCompositionFeature = 0x2A9B,
        BodyCompositionMeasurement = 0x2A9C,
        WeightMeasurement = 0x2A9D,
        WeightScaleFeature = 0x2A9E,
        UserControlPoint = 0x2A9F,
        MagneticFluxDensity2D = 0x2AA0,
        MagneticFluxDensity3D = 0x2AA1,
        Language = 0x2AA2,
        BarometricPressureTrend = 0x2AA3,
        BondManagementControlPoint = 0x2AA4,
        BondManagementFeature = 0x2AA5,
        CentralAddressResolution = 0x2AA6,
        CgmMeasurement = 0x2AA7,
        CgmFeature = 0x2AA8,
        CgmStatus = 0x2AA9,
        CgmSessionStartTime = 0x2AAA,
        CgmSessionRunTime = 0x2AAB,
        CgmSpecificOpsControlPoint = 0x2AAC,
        IndoorPositioningConfiguration = 0x2AAD,
        Latitude = 0x2AAE,
        Longitude = 0x2AAF,
        LocalNorthCoordinate = 0x2AB0,
        LocalEastCoordinate = 0x2AB1,
        FloorNumber = 0x2AB2,
        Altitude = 0x2AB3,
        Uncertainty = 0x2AB4,
        LocationName = 0x2AB5,
        Uri = 0x2AB6,
        HttpHeaders = 0x2AB7,
        HttpStatusCode = 0x2AB8,
        HttpEntityBody = 0x2AB9,
        HttpControlPoint = 0x2ABA,
        HttpsSecurity = 0x2ABB,
        TdsControlPoint = 0x2ABC,
        OtsFeature = 0x2ABD,
        ObjectName = 0x2ABE,
        ObjectType = 0x2ABF,
        ObjectSize = 0x2AC0,
        ObjectFirstCreated = 0x2AC1,
        ObjectLastModified = 0x2AC2,
        ObjectId = 0x2AC3,
        ObjectProperties = 0x2AC4,
        ObjectActioncontrolPoint = 0x2AC5,
        ObjectListControlPoint = 0x2AC6,
        ObjectListFilter = 0x2AC7,
        ObjectChanged = 0x2AC8,
        ResolvablePrivateAddressOnly = 0x2AC9,
        Unspecified = 0x2ACA,
        DirectoryListing = 0x2ACB,
        FitnessMachineFeature = 0x2ACC,
        TreadmillData = 0x2ACD,
        CrossTrainerData = 0x2ACE,
        StepClimberData = 0x2ACF,
        StairClimberData = 0x2AD0,
        RowerData = 0x2AD1,
        IndoorBikeData = 0x2AD2,
        TrainingStatus = 0x2AD3,
        SupportedSpeedRange = 0x2AD4,
        SupportedInclinationRange = 0x2AD5,
        SupportedResistanceLevelRange = 0x2AD6,
        SupportedHeartRateRange = 0x2AD7,
        SupportedPowerRange = 0x2AD8,
        FitnessMachineControlPoint = 0x2AD9,
        FitnessMachineStatus = 0x2ADA,
        MeshProvisioningDataIn = 0x2ADB,
        MeshProvisioningDataOut = 0x2ADC,
        MeshProxyDataIn = 0x2ADD,
        MeshProxyDataOut = 0x2ADE,
        AverageCurrent = 0x2AE0,
        AverageVoltage = 0x2AE1,
        Boolean = 0x2AE2,
        ChromaticDistanceFromPlanckian = 0x2AE3,
        ChromaticityCoordinates = 0x2AE4,
        ChromaticityInCctAndDuvValues = 0x2AE5,
        ChromaticityTolerance = 0x2AE6,
        Cie13_3_1995ColorRenderingIndex = 0x2AE7,
        Coefficient = 0x2AE8,
        CorrelatedColorTemperature = 0x2AE9,
        Count16 = 0x2AEA,
        Count24 = 0x2AEB,
        CountryCode = 0x2AEC,
        DateUtc = 0x2AED,
        ElectricCurrent = 0x2AEE,
        ElectricCurrentRange = 0x2AEF,
        ElectricCurrentSpecification = 0x2AF0,
        ElectricCurrentStatistics = 0x2AF1,
        Energy = 0x2AF2,
        EnergyInAPeriodOfDay = 0x2AF3,
        EventStatistics = 0x2AF4,
        FixedString16 = 0x2AF5,
        FixedString24 = 0x2AF6,
        FixedString36 = 0x2AF7,
        FixedString8 = 0x2AF8,
        GenericLevel = 0x2AF9,
        GlobalTradeItemNumber = 0x2AFA,
        Illuminance = 0x2AFB,
        LuminousEfficacy = 0x2AFC,
        LuminousEnergy = 0x2AFD,
        LuminousExposure = 0x2AFE,
        LuminousFlux = 0x2AFF,
        LuminousFluxRange = 0x2B00,
        LuminousIntensity = 0x2B01,
        MassFlow = 0x2B02,
        PerceivedLightness = 0x2B03,
        Percentage8 = 0x2B04,
        Power = 0x2B05,
        PowerSpecification = 0x2B06,
        RelativeRuntimeInACurrentRange = 0x2B07,
        RelativeRuntimeInAGenericLevelRange = 0x2B08,
        RelativeValueInAVoltageRange = 0x2B09,
        RelativeValueInAnIlluminanceRange = 0x2B0A,
        RelativeValueInAPeriodOfDay = 0x2B0B,
        RelativeValueInATemperatureRange = 0x2B0C,
        Temperature8 = 0x2B0D,
        Temperature8InAPeriodOfDay = 0x2B0E,
        Temperature8Statistics = 0x2B0F,
        TemperatureRange = 0x2B10,
        TemperatureStatistics = 0x2B11,
        TimeDecihour8 = 0x2B12,
        TimeExponential8 = 0x2B13,
        TimeHour24 = 0x2B14,
        TimeMillisecond24 = 0x2B15,
        TimeSecond16 = 0x2B16,
        TimeSecond8 = 0x2B17,
        Voltage = 0x2B18,
        VoltageSpecification = 0x2B19,
        VoltageStatistics = 0x2B1A,
        VolumeFlow = 0x2B1B,
        ChromaticityCoordinate = 0x2B1C,
        RcFeature = 0x2B1D,
        RcSettings = 0x2B1E,
        ReconnectionConfigurationControlPoint = 0x2B1F,
        IddStatusChanged = 0x2B20,
        IddStatus = 0x2B21,
        IddAnnunciationStatus = 0x2B22,
        IddFeatures = 0x2B23,
        IddStatusReaderControlPoint = 0x2B24,
        IddCommandControlPoint = 0x2B25,
        IddCommandData = 0x2B26,
        IddRecordAccessControlPoint = 0x2B27,
        IddHistoryData = 0x2B28,
        ClientSupportedFeatures = 0x2B29,
        DatabaseHash = 0x2B2A,
        BssControlPoint = 0x2B2B,
        BssResponse = 0x2B2C,
        EmergencyId = 0x2B2D,
        EmergencyText = 0x2B2E,
        ServerSupportedFeatures = 0x2B3A,
        HM10 = 0xFFE1,
    };
}

#endif // BLE_SERIAL_INCLUDE_BLUETOOTH_HPP_
