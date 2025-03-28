#include <Arduino.h>
#include <ST7735S_Display.h>
#include <Wire.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>

// Pines I2C para ESP32
const int SDA_Pin = 21; // Pin SDA
const int SCL_Pin = 22; // Pin SCL
// Pines para botones
const int BUT1 = 35; // Pin Button 1
const int BUT2 = 32; // Pin Button 2
const int BUT3 = 33; // Pin Button 3
// Pines para la tarjeta SD
const int SD_CS = 5;   // Chip Select en GPIO 5
const int SD_SCK = 4; // SCK en GPIO 4
const int SD_MISO = 16; // MISO en GPIO 16
const int SD_MOSI = 17; // MOSI en GPIO 17
// Pines para la pantalla TFT
const int TFT_CS = 14; // Chip select pin
const int TFT_RST = 26; // Reset pin
const int TFT_DC = 27; // Data/Command pin
const int TFT_BL = 25; // Backlight pin
const int TFT_SCK = 12; // SCK pin
const int TFT_MOSI = 13; // MOSI pin

// Dirección y registros del ADXL345
#define ADXL345 0x53           // Dirección I2C del ADXL345
#define REG_POWER_CTL 0x2D     // Registro de control de energía
#define REG_DATA_FORMAT 0x31   // Registro de formato de datos
#define REG_DATAX0 0x32        // Inicio de datos del eje X
#define REG_DATAY0 0x34        // Inicio de datos del eje Y
#define REG_DATAZ0 0x36        // Inicio de datos del eje Z

// Variables globales
const float scale = 256.0; // Factor de escala para convertir a g
const float alpha = 0.4;             // Filtro de paso bajo exponencial (0 a 1; menor = más suavizado)
const float deadband = 3.0;          // Banda muerta en grados para evitar movimientos insignificantes
const float controlThreshold = 90.0; // Se usa 90° de error para mapear a 6000 Hz
const float emergencyThreshold = 45.0; // Umbral de emergencia en Pitch
int16_t prevCommand = 0;
uint16_t prevFrequency = 0;
float desiredAngle = 0.00;
float frstposRoll = 0.00;
float filteredRoll = 0.0, filteredPitch = 0.0;
const uint16_t maxFrequency = 1000;    // Frecuencia máxima en Hz
const uint16_t minFrequency = 160;      // Frecuencia mínima en Hz
unsigned long startTime = 0;
float tmp = 0.00;
float count = 0.00;
int lstBUT1 = HIGH, lstBUT2 = HIGH, lstBUT3 = HIGH;
bool stt3 = false;
bool cnt = true;
bool prevcnt = false;
unsigned long lastDebounceTime1 = 0, lastDebounceTime2 = 0, lastDebounceTime3 = 0;
const unsigned long debounceDelay = 50; // Tiempo de debounce en ms
float V1 = 0.00;
float V2 = 0.00;
float V3 = 0.00;
static unsigned long lastUpdate = 0; // Última actualización
unsigned long lastDisplayUpdate = 0; // Última actualización de la pantalla
const unsigned long updateInterval = 100;  // 100 ms entre actualizaciones
unsigned long buttonHoldStartTime1 = 0;
unsigned long buttonHoldStartTime2 = 0;
const unsigned long repeatDelay = 500;  // Tiempo antes de repetir (ms)
const unsigned long repeatInterval = 100; // Intervalo de repetición (ms)
float accelerationFactor = 1.0;
char userInput[20];

// Crear instancia de la clase SPI para la tarjeta SD
SPIClass sd_SPI(HSPI);
// Crear instancia de la clase ST7735S_Display para la pantalla TFT
ST7735S_Display tft(TFT_CS, TFT_RST, TFT_DC, TFT_BL, TFT_SCK, TFT_MOSI);


// ===================== Función: Leer un eje del ADXL345 =====================
int16_t readAxis(uint8_t reg) {
  Wire.beginTransmission(ADXL345);
  Wire.write(reg);
  Wire.endTransmission(false); // Repeated start
  Wire.requestFrom((uint8_t)ADXL345, (uint8_t)2);
  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();
  return (int16_t)(msb << 8 | lsb);
}

// ===================== Función: Filtro de Paso Bajo Exponencial =====================
float lowPassFilter(float newVal, float prevVal) {
  return alpha * newVal + (1 - alpha) * prevVal;
}

// ===================== Función: Calcular CRC16 para Modbus =====================
uint16_t calculateCRC(uint8_t* data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// ===================== Función: Enviar Trama Modbus de Dirección =====================
void sendModbusDirection(int16_t command) {
  uint8_t data[8];
  data[0] = 0x01; // ID del esclavo
  data[1] = 0x06; // Función: Write Single Register
  data[2] = 0x20; // Registro alta (0x2000 para dirección de giro)
  data[3] = 0x00; // Registro baja
  data[4] = (command >> 8) & 0xFF; // Byte alto del comando
  data[5] = command & 0xFF;        // Byte bajo del comando
  uint16_t crc = calculateCRC(data, 6);
  data[6] = (uint8_t)(crc & 0xFF); // CRC byte bajo
  data[7] = (uint8_t)(crc >> 8);   // CRC byte alto
  Serial.write(data, 8);           // Enviar la trama
}

// ===================== Función: Enviar Trama Modbus de Frecuencia =====================
void sendModbusFrequency(uint16_t frequency) {
  uint8_t data[8];
  data[0] = 0x01; // ID del esclavo
  data[1] = 0x06; // Función: Write Single Register
  data[2] = 0x20; // Registro alta (0x2001 para frecuencia)
  data[3] = 0x01; // Registro baja
  data[4] = (uint8_t)(frequency >> 8); // Byte alto de la frecuencia
  data[5] = (uint8_t)frequency;        // Byte bajo de la frecuencia
  uint16_t crc = calculateCRC(data, 6);
  data[6] = (uint8_t)(crc & 0xFF); // CRC byte bajo
  data[7] = (uint8_t)(crc >> 8);   // CRC byte alto
  Serial.write(data, 8);           // Enviar la trama
}

// ===================== Función: Enviar Trama de Paro de Emergencia =====================
void sendEmergencyStop() {
  uint8_t data[8];
  data[0] = 0x01; // ID del esclavo
  data[1] = 0x06; // Función: Write Single Register
  data[2] = 0x20; // Registro alta (0x2000 para dirección de giro)
  data[3] = 0x00; // Registro baja
  data[4] = 0x00; // Dato alto (0 para detener movimiento)
  data[5] = 0x01; // Dato bajo (ejemplo: 0x0001 para paro de emergencia)
  uint16_t crc = calculateCRC(data, 6);
  data[6] = (uint8_t)(crc & 0xFF);
  data[7] = (uint8_t)(crc >> 8);
  Serial.write(data, 8);
}

// ===================== Función: Ingresar Ángulo =====================
void enterAngle() {
  unsigned long currentTime = millis();
  bool button1Pressed = digitalRead(BUT1) == LOW;  // Definir estado de botones
  bool button2Pressed = digitalRead(BUT2) == LOW;
  bool button3Pressed = digitalRead(BUT3) == LOW;
  bool needsUpdate = false;  // Bandera para cambios

  // Lectura y debounce del botón 1 (incrementar)
  if (button1Pressed) {
    if (lstBUT1 == HIGH && (currentTime - lastDebounceTime1) > debounceDelay) {
        count += 0.5;
        buttonHoldStartTime1 = currentTime;
        accelerationFactor = 1.0;
        lastDebounceTime1 = currentTime;
        needsUpdate = true;
    } 
    else if (currentTime - buttonHoldStartTime1 > repeatDelay) {
        if ((currentTime - lastDebounceTime1) > (repeatInterval / accelerationFactor)) {
            count += 0.5;
            lastDebounceTime1 = currentTime;
            accelerationFactor = min(accelerationFactor * 1.2, 5.0);
            needsUpdate = true;
        }
    }
    lstBUT1 = LOW;
  } else {
      if (lstBUT1 == LOW) accelerationFactor = 1.0;
      lstBUT1 = HIGH;
  }
  // Lectura y debounce del botón 2 (decrementar)
  if (button2Pressed) {
    if (lstBUT2 == HIGH && (currentTime - lastDebounceTime2) > debounceDelay) {
        count -= 0.5;
        buttonHoldStartTime2 = currentTime;
        accelerationFactor = 1.0;
        lastDebounceTime2 = currentTime;
        needsUpdate = true;
    } 
    else if (currentTime - buttonHoldStartTime2 > repeatDelay) {
        if ((currentTime - lastDebounceTime2) > (repeatInterval / accelerationFactor)) {
            count -= 0.5;
            lastDebounceTime2 = currentTime;
            accelerationFactor = min(accelerationFactor * 1.2, 5.0);
            needsUpdate = true;
        }
    }
    lstBUT2 = LOW;
  } else {
      if (lstBUT2 == LOW) accelerationFactor = 1.0;
      lstBUT2 = HIGH;
  }

  // Lectura y debounce del botón 3 (confirmar entrada)
  if (button3Pressed) {
    if (lstBUT3 == HIGH && (currentTime - lastDebounceTime3) > debounceDelay) {
        startTime = millis();  // Reiniciar temporizador
        stt3 = true;          // Cambiar estado de confirmación
        lastDebounceTime3 = currentTime;
        needsUpdate = true;    // Forzar actualización visual
        
        // Opcional: feedback visual al confirmar
        V3 = millis() - startTime; // Ejemplo: mostrar tiempo
    }
    lstBUT3 = LOW;
} else {
    lstBUT3 = HIGH;
}
  // Limitar valores y actualizar display
  // Actualización eficiente de la pantalla
  count = constrain(count, -90.0, 90.0);  // Limitar rango
    if (needsUpdate || (currentTime - lastDisplayUpdate) > updateInterval) {
        desiredAngle = count;
        V1 = desiredAngle;
        V2 = 0.00;  // Asegúrate de que esta variable existe
        V3 = 0.00;
        tft.setCustomLabel("Posicion_Deseada:"); // Cambiar texto
        tft.updateDisplay(V1, V2, V3);
        lastDisplayUpdate = currentTime;
    }
}

// ===================== setup() =====================
void setup() {
  Serial.begin(9600, SERIAL_8N1);   // Inicializa el monitor serie a 9600 baudios
  Wire.begin(SDA_Pin, SCL_Pin);     // Inicializa I2C en los pines 21 y 22
  sd_SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); // Iniciar SPI
  pinMode(BUT1, INPUT_PULLUP); // Configura los botones como entrada con pull-up
  pinMode(BUT2, INPUT_PULLUP); // Configura los botones como entrada con pull-up
  pinMode(BUT3, INPUT_PULLUP); // Configura los botones como entrada con pull-up

  // Configurar el ADXL345:
  Wire.beginTransmission(ADXL345); // Inicia la transmisión
  Wire.write(REG_POWER_CTL); // Escribe en el registro de control de energía
  Wire.write(0x08);  // Modo medición
  Wire.endTransmission(); // Finaliza la transmisión
  
  Wire.beginTransmission(ADXL345); // Inicia la transmisión
  Wire.write(REG_DATA_FORMAT); // Escribe en el registro de formato de datos
  Wire.write(0x08);  // Configura a ±2g
  Wire.endTransmission(); // Finaliza la transmisión
  
  Wire.beginTransmission(ADXL345); // Inicia la transmisión
  Wire.write(0x2C); // Registro de tasa de muestreo
  Wire.write(0x0A);  // Configura a 100 Hz
  Wire.endTransmission(); // Finaliza la transmisión

  delay(100); // Espera a que se estabilice el sensor

  // Inicializar SD con SPI personalizado
  if (!SD.begin(SD_CS, sd_SPI)) {
    Serial.println("Error al inicializar la tarjeta SD."); // Error al inicializar
    return; // Terminar el programa
  }
  Serial.println("Tarjeta SD inicializada correctamente."); // Inicialización exitosa
  // Crear instancia para la pantalla
  tft.begin(); // Iniciar la pantalla TFT
  tft.fillScreen(0x0000); // Limpiar la pantalla

  // Inicializar los valores filtrados con la primera lectura para evitar arranques erráticos
  int16_t initX = readAxis(REG_DATAX0); // Leer el eje X
  int16_t initY = readAxis(REG_DATAY0); // Leer el eje Y
  int16_t initZ = readAxis(REG_DATAZ0); // Leer el eje Z
  float ax = initX / scale; // Convertir a g
  float ay = initY / scale; // Convertir a g
  float az = initZ / scale; // Convertir a g
  filteredRoll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI; // Calcular Roll
  filteredPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI; // Calcular Pitch
  // Solicitar al usuario el ángulo deseado inicial y bloquear hasta recibirlo
  startTime = millis(); // Inicia el contador de tiempo
}

// ===================== loop() =====================
void loop() {
  while (stt3 == false) { // Mientras no se confirme el ángulo
    enterAngle(); // Llamar a la función para ingresar el ángulo
  }
  // Leer datos crudos del acelerómetro
  int16_t rawX = readAxis(REG_DATAX0); // Leer el eje X
  int16_t rawY = readAxis(REG_DATAY0); // Leer el eje Y
  int16_t rawZ = readAxis(REG_DATAZ0); // Leer el eje Z
  // Convertir a aceleración en g
  float ax = rawX / scale; // Convertir a g
  float ay = rawY / scale; // Convertir a g
  float az = rawZ / scale; // Convertir a g
  // Calcular Roll y Pitch en grados
  float roll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI; // Calcular Roll
  float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * -1.0 *180.0 / PI; // Calcular Pitch
  // Aplicar filtro de paso bajo para suavizar los ángulos
  filteredRoll = lowPassFilter(roll, filteredRoll); // Filtrar Roll
  filteredPitch = lowPassFilter(pitch, filteredPitch); // Filtrar Pitch

  if (cnt != prevcnt){ // Si el estado de cnt ha cambiado
    frstposRoll = filteredRoll; // Guardar la posición inicial
    prevcnt = cnt; // Actualizar el estado anterior
  } 

  // Calcular el error entre el ángulo deseado y el ángulo actual (Roll)
  float error = desiredAngle - filteredRoll; // Error en Roll
  float absError = fabs(error); // Valor absoluto del error
  // Calculo de la frecuencia de control
  uint16_t frequency = (uint16_t)(maxFrequency * (absError / controlThreshold)); // Frecuencia de control
  if (frequency > maxFrequency) frequency = maxFrequency; // Limitar a la frecuencia máxima
  if (frequency < minFrequency) frequency = minFrequency; // Limitar a la frecuencia mínima

  // Determinar el comando de dirección basado en el error:
  int16_t command = 0;  
  command = (error > 0) ? 0x12 : 0x22; // 0x12 para adelante, 0x22 para atrás
  // Si el ángulo de Pitch supera el umbral de emergencia, enviar paro de emergencia
  if (fabs(filteredPitch) >= emergencyThreshold) {
      sendEmergencyStop();
      prevCommand = 0;
      prevFrequency = 0;
      startTime = millis();
  } else {
    // Enviar comando de dirección si ha cambiado
    if (command != prevCommand) {
      sendModbusDirection(command); // Enviar dirección
      prevCommand = command; // Actualizar el comando previo
    }
    delay(200); // Esperar un poco para evitar enviar comandos demasiado rápido
    // Enviar frecuencia si ha cambiado significativamente (diferencia de 5 Hz o más)
    if (abs(frequency - prevFrequency) >= 5) {
      sendModbusFrequency(frequency); // Enviar frecuencia
      prevFrequency = frequency; // Actualizar la frecuencia previa

    }
  }
  V2 = filteredRoll;
  if(millis() - lastUpdate >= updateInterval) {
    tft.updateDisplay(V1, V2, V3); // Actualizar la pantalla TFT
    lastUpdate = millis(); // Actualizar el tiempo de la última actualización
} 

  if (absError < 0.5) { // Si la diferencia es menor a 0.5°, se considera alcanzada la posición
    sendEmergencyStop(); // Enviar paro de emergencia
    delay(200); // Esperar un poco para evitar enviar comandos demasiado rápido
    prevCommand = 0; // Reiniciar el comando previo
    prevFrequency = 0; // Reiniciar la frecuencia previa
    sendModbusDirection(0x00); // Detener el motor 
    sendModbusFrequency(0); // Detener la frecuencia
    delay(200); // Esperar un poco para evitar enviar comandos demasiado rápido
    unsigned long elapsedTime = millis() - startTime; // Calcular el tiempo transcurrido
    tmp = elapsedTime / 1000.0; // Convertir a segundos
   // Actualizar la pantalla con los valores actuales
    if(millis() - lastUpdate >= updateInterval) {
      V1 = frstposRoll; // Posición inicial
      V2 = filteredRoll; // Posición final
      V3 = tmp; // Tiempo
      tft.setCustomLabel("Posicion_Inicial:");
      tft.updateDisplay(V1, V2, V3); // Actualizar la pantalla TFT
      lastUpdate = millis(); // Actualizar el tiempo de la última actualización
    }
    // Guardar los datos en la tarjeta SD
    File dataFile = SD.open("/datos.txt", FILE_APPEND); // Abrir archivo en modo de escritura
      if (dataFile) {
        dataFile.print("Posición inicial: "); // Escribir en el archivo
        dataFile.print(frstposRoll); // Escribir posición inicial
        dataFile.print(" Posición Final: "); // Escribir en el archivo
        dataFile.print(filteredRoll); // Escribir posición final
        dataFile.print(" deg, Tiempo: "); // Escribir en el archivo
        dataFile.println(tmp); // Escribir tiempo
        dataFile.close(); // Cerrar el archivo
      }  else {
        Serial.println("Error al abrir el archivo."); // Error al abrir el archivo
      }
    cnt = !cnt; // Cambiar el estado de cnt
    delay(10000); // Esperar 10 segundos antes de permitir una nueva entrada
    if(millis() - lastUpdate >= updateInterval) {
      V1 = 0.00; // Reiniciar posición inicial
      V2 = 0.00; // Reiniciar posición final
      V3 = 0.00; // Reiniciar tiempo
      tft.setCustomLabel("Posicion_Deseada:");
      tft.updateDisplay(V1, V2, V3); // Actualizar la pantalla TFT
      lastUpdate = millis(); // Actualizar el tiempo de la última actualización
    }
    startTime = millis(); // Reiniciar el temporizador
    stt3 = false; // Reiniciar el estado
    count = 0.00; // Reiniciar el ángulo deseado
    while (stt3 == false) { // Mientras no se confirme el ángulo
      enterAngle(); // Llamar a la función para ingresar el ángulo
    }
  } 
  delay(10); // Pequeña pausa para evitar lecturas rápidas no deseadas
}