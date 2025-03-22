#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <algorithm> // Añadir esta línea

const int SDA_Pin = 21; // SDA pin
const int SCL_Pin = 22; // SCL pin
#define ADXL345 0x53 // ADXL345 I2C address
#define REG_POWER_CTL 0x2D // Power Control Register
#define REG_DATA_FORMAT 0x31 // Data Format Register
#define REG_DATAX0 0x32 // X-Axis Data 0
#define REG_DATAY0 0x34 // Y-Axis Data 0
#define REG_DATAZ0 0x36 // Z-Axis Data 0

const float alpha = 0.1; // Low pass filter constant
const float deadband = 5.0; // Deadband for control
const float maxFrequency = 60.0; // Max motor frequency in Hz
const float controlThreshold = 90.0; // Max roll angle

float filteredX = 0.0, filteredY = 0.0, filteredZ = 0.0;
float rollAngle = 0.0;
int targetAngle = 0;

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_Pin, SCL_Pin);
    
    // Configure ADXL345
    Wire.beginTransmission(ADXL345);
    Wire.write(REG_POWER_CTL);
    Wire.write(0x08); // Enable measurement
    Wire.endTransmission();
    
    Wire.beginTransmission(ADXL345);
    Wire.write(REG_DATA_FORMAT);
    Wire.write(0x08); // +/-2g range
    Wire.endTransmission();
}

void loop() {
    int16_t rawX = readAxis(REG_DATAX0);
    int16_t rawY = readAxis(REG_DATAY0);
    int16_t rawZ = readAxis(REG_DATAZ0);

    filteredX = lowPassFilter(rawX * 0.0039, filteredX);
    filteredZ = lowPassFilter(rawZ * 0.0039, filteredZ);
    rollAngle = atan2(filteredX, filteredZ) * 180 / PI;

    Serial.print("Roll Angle: ");
    Serial.println(rollAngle);

    adjustMotorSpeed();
    delay(100);
}

int16_t readAxis(uint8_t reg) {
    Wire.beginTransmission(ADXL345);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(ADXL345, 2);
    return (Wire.read() | (Wire.read() << 8));
}

float lowPassFilter(float newVal, float prevVal) {
    return alpha * newVal + (1 - alpha) * prevVal;
}

void adjustMotorSpeed() {
    float error = targetAngle - rollAngle;
    int direction = (error > 0) ? 0x12 : 0x22;
    float normalizedError = std::min(static_cast<float>(abs(error) / controlThreshold), 1.0f); // Añadir std:: antes de min
    int frequency = normalizedError * maxFrequency;
    
    if (abs(error) < deadband) {
        frequency = 0;
    }
    
    sendModbusDirection(direction);
    sendModbusFrequency(frequency);
    Serial.print("Direction: ");
    Serial.print(direction);
    Serial.print(" Frequency: ");
    Serial.println(frequency);
}

void sendModbusDirection(int16_t command) {
    uint8_t data[8] = {0x01, 0x06, 0x20, 0x00, (uint8_t)(command >> 8), (uint8_t)command, 0x00, 0x00};
    Serial.write(data, 8);
}

void sendModbusFrequency(uint16_t frequency) {
    uint8_t data[8] = {0x01, 0x06, 0x20, 0x01, (uint8_t)(frequency >> 8), (uint8_t)frequency, 0x00, 0x00};
    Serial.write(data, 8);
}
