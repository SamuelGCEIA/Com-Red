#ifndef ST7735S_DISPLAY_H
#define ST7735S_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>

class ST7735S_Display {
public:
    // Constructor con pines personalizables
    ST7735S_Display(uint8_t cs, uint8_t rst, uint8_t dc, uint8_t bl, uint8_t sck, uint8_t mosi);
    
    // Métodos públicos
    void begin();
    void updateDisplay(float v1, float v2, float v3);
    void fillScreen(uint16_t color);
    void setRotation(uint8_t rotation);
    void setCustomLabel(const char* newLabel); // Método para cambiar el texto
    
private:
    // Pines
    uint8_t _cs, _rst, _dc, _bl, _sck, _mosi;
    // Variables para el texto
    char lastText4[10] = "";
    char lastText5[10] = "";
    char lastText6[10] = "";
    bool firstUpdate = true;

    char customLabel[20] = "Posicion_Deseada:"; // Buffer para texto personalizable
    
    // Métodos privados
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void sendColor(uint16_t color);
    void setAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
    void drawPixel(uint8_t x, uint8_t y, uint16_t color);
    void drawChar(char c, uint8_t x, uint8_t y, uint16_t color);
    void drawCharScaled(char c, uint8_t x, uint8_t y, uint16_t color, uint8_t scale);
    void drawTextRotated(const char *text, uint8_t x, uint8_t y, uint16_t color, uint8_t scale);
    void updateTextArea(uint8_t x, uint8_t y, const char* oldText, const char* newText, uint8_t scale, uint16_t bgColor, uint16_t textColor);
};

#endif