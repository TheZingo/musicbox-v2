#include "BatteryHandler.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <iostream>

#define INA219_ADDRESS 0x41

BatteryHandler::BatteryHandler() {
    i2cFile = open("/dev/i2c-1", O_RDWR);
    if (i2cFile < 0) {
        throw std::runtime_error("Failed to open I2C bus");
    }

    if (ioctl(i2cFile, I2C_SLAVE, INA219_ADDRESS) < 0) {
        throw std::runtime_error("Failed to acquire bus access and/or talk to slave");
    }

    calibrate();
}

BatteryHandler::~BatteryHandler() {
    if(i2cFile > 0) {
        close(i2cFile);
    }
}

uint16_t BatteryHandler::readRegister(uint8_t reg) {
    uint8_t data[2];
    data[0] = reg;
    write(i2cFile, data, 1);
    read(i2cFile, data, 2);
    return (data[0] << 8) | data[1];
}

void BatteryHandler::writeRegister(uint8_t reg, uint16_t value) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
    write(i2cFile, data, 3);
}

void BatteryHandler::calibrate() {
    float shunt_resistance = 0.1; // [Ohm] Resistor value 
    currentLSB = .1524;
    powerLSB = currentLSB * 0.02;
        
    calibrationValue = static_cast<uint16_t>((0.04096 / (currentLSB * shunt_resistance)) * 10000);
    writeRegister(CALIBRATION_REGISTER, calibrationValue);
        
    // Configure the INA219
    #define RANGE_16V 0x00
    #define GAIN_2_80MV 0x01
    #define GAIN_8_320MV 0x03
    #define ADC_RES_12BIT_32S 0x0D
    #define SANDBVOLT_CONTINUOUS 0x07

    uint16_t config = (RANGE_16V << 13 | GAIN_2_80MV << 11 | ADC_RES_12BIT_32S << 7 | ADC_RES_12BIT_32S << 3 | SANDBVOLT_CONTINUOUS);
    writeRegister(CONFIG_REGISTER, config);
}

float BatteryHandler::getBusVoltage() {
    writeRegister(CALIBRATION_REGISTER, calibrationValue);
    uint16_t value = readRegister(BUS_VOLTAGE_REGISTER);
    return (value >> 3) * 0.004;
}

float BatteryHandler::getShuntVoltage() {
    writeRegister(CALIBRATION_REGISTER, calibrationValue);
    uint16_t value = readRegister(SHUNT_VOLTAGE_REGISTER);
    int16_t shunt = value;
    if(shunt > 32767) {
        shunt -= 65536;
    }
    return shunt * 0.00001;
}

float BatteryHandler::getCurrent() {
    uint16_t value = readRegister(CURRENT_REGISTER);
    int16_t current = value;
    if(current > 32767) {
        current -= 65536;
    }
    return current * currentLSB * 0.001;
}

float BatteryHandler::getPower(){
    uint16_t value = readRegister(POWER_REGISTER);
    int16_t power = value;
    if(power > 32767) {
        power -= 65536;
    }
    return power * powerLSB;
}

float BatteryHandler::getBatteryPercentage() {
    float busVoltage = getBusVoltage();
    float p = (busVoltage - 9) / 3.6 * 100;
    if(p < 0) {
        return 0;
    } else if(p > 100) {
        return 100;
    } else {
        return p;
    }
}

bool BatteryHandler::isBatteryLow() {
    return getBatteryPercentage() < 10;
}

bool BatteryHandler::isBatteryCharging() {
    return getCurrent() > 0;
}

void BatteryHandler::monitorBattery() {
    std::unique_lock<std::mutex> lk(cv_m);
    while (monitoringActive) {
        float batteryPercentage = getBatteryPercentage();
        if (batteryPercentage < 3.0) {
            system("sudo poweroff");
        }
        cv.wait_for(lk, std::chrono::minutes(5), [this] { return !monitoringActive; });
    }
}

void BatteryHandler::startMonitoring() {
    monitoringActive = true;
    monitoringThread = std::thread(&BatteryHandler::monitorBattery, this);
}

void BatteryHandler::endMonitoring() {
    {
        std::lock_guard<std::mutex> lk(cv_m);
        monitoringActive = false;
    }
    cv.notify_all();
    if (monitoringThread.joinable()) {
        monitoringThread.join();
    }
}