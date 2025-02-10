#ifndef INCLUDE_BATTERYHANDLER_H_
#define INCLUDE_BATTERYHANDLER_H_

#include <cstdint>
#include <thread>
#include <atomic>
#include <condition_variable>

class BatteryHandler
{
    public:
        BatteryHandler();
        ~BatteryHandler();
        bool isBatteryLow();
        bool isBatteryCharging();
        float getBatteryPercentage();
        float getBusVoltage();
        float getShuntVoltage();
        float getCurrent();
        float getPower();
        void startMonitoring();
        void endMonitoring();

    private:
        // Register addresses for the INA219
        static constexpr uint8_t CONFIG_REGISTER = 0x00;
        static constexpr uint8_t SHUNT_VOLTAGE_REGISTER = 0x01;
        static constexpr uint8_t BUS_VOLTAGE_REGISTER = 0x02;
        static constexpr uint8_t POWER_REGISTER = 0x03;
        static constexpr uint8_t CURRENT_REGISTER = 0x04;
        static constexpr uint8_t CALIBRATION_REGISTER = 0x05;

        int i2cFile;
        uint16_t calibrationValue;
        float currentLSB;
        float powerLSB;
        std::thread monitoringThread;
        std::atomic<bool> monitoringActive{false};
        std::condition_variable cv;
        std::mutex cv_m;

        uint16_t readRegister(uint8_t reg);
        void writeRegister(uint8_t reg, uint16_t value);
        void calibrate();
        void monitorBattery();

};

#endif