#include "ST7735S_Display.h"
#include <SPI.h>
#include <Arduino.h>

// Definiciones de comandos
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT 0x11
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36


void ST7735S_Display::writeCommand(uint8_t cmd)
{
  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  SPI.transfer(cmd);
  digitalWrite(_cs, HIGH);
}

void ST7735S_Display::writeData(uint8_t data)
{
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  SPI.transfer(data);
  digitalWrite(_cs, HIGH);
}

void ST7735S_Display::fillScreen(uint16_t color) {
    setAddressWindow(0, 0, 127, 159);
    for (uint32_t i = 0; i < 128 * 160; i++) {
        sendColor(color);
    }
}

void ST7735S_Display::setAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    writeCommand(ST7735_CASET);
    writeData(0x00);
    writeData(x0);
    writeData(0x00);
    writeData(x1);

    writeCommand(ST7735_RASET);
    writeData(0x00);
    writeData(y0);
    writeData(0x00);
    writeData(y1);

    writeCommand(ST7735_RAMWR);
}

void ST7735S_Display::drawPixel(uint8_t x, uint8_t y, uint16_t color)
{
  setAddressWindow(x, y, x, y);
  sendColor(color);
}

// Fuente ASCII 5x7 (igual que en tu código original)
const uint8_t font5x7[94][5] = {
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !  1
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "  2
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #  3
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $  4
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %  5
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &  6
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '  7
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, //
};

void ST7735S_Display::drawChar(char c, uint8_t x, uint8_t y, uint16_t color)
{
  if (c < 32 || c > 127)
    c = '?'; // Caracter no soportado
  for (uint8_t i = 0; i < 5; i++)
  {
    uint8_t line = font5x7[c - 33][i];
    for (uint8_t j = 0; j < 7; j++)
    {
      if (line & 0x01)
        drawPixel(x + i, y + j, color);
      line >>= 1;
    }
  }
}

void ST7735S_Display::drawCharScaled(char c, uint8_t x, uint8_t y, uint16_t color, uint8_t scale)
{
  if (c < 32 || c > 127)
    c = '?';
  for (uint8_t i = 0; i < 5; i++)
  {
    uint8_t line = font5x7[c - 33][i];
    for (uint8_t j = 0; j < 7; j++)
    {
      if (line & 0x01)
      {
        // Dibujar píxeles más grandes
        for (uint8_t dx = 0; dx < scale; dx++)
        {
          for (uint8_t dy = 0; dy < scale; dy++)
          {
            drawPixel(x + i * scale + dx, y + j * scale + dy, color);
          }
        }
      }
      line >>= 1;
    }
  }
}

void ST7735S_Display::drawTextRotated(const char *text, uint8_t x, uint8_t y, uint16_t color, uint8_t scale)
{
  uint8_t textHeight = strlen(text) * 6 * scale; // Alto del texto rotado
  uint8_t textWidth = 7 * scale;                 // Ancho del texto rotado

  // Ajustar la posición inicial para centrar el texto rotado
  x = x - textWidth / 2;
  y = y - textHeight / 2;

  for (uint8_t i = 0; i < strlen(text); i++)
  {
    char c = text[i];
    if (c < 32 || c > 127)
      c = '?'; // Caracter no soportado
    for (uint8_t j = 0; j < 5; j++)
    {
      uint8_t line = font5x7[c - 33][j];
      for (uint8_t k = 0; k < 7; k++)
      {
        if (line & 0x01)
        {
          // Dibujar píxeles rotados
          for (uint8_t dx = 0; dx < scale; dx++)
          {
            for (uint8_t dy = 0; dy < scale; dy++)
            {
              drawPixel(x + (6 - k) * scale + dx, y + (i * 6 * scale) + j * scale + dy, color);
            }
          }
        }
        line >>= 1;
      }
    }
  }
}


void ST7735S_Display::sendColor(uint16_t color) {
    writeData(color >> 8);   // High byte
    writeData(color & 0xFF); // Low byte
}

void ST7735S_Display::setRotation(uint8_t rotation) {
    writeCommand(ST7735_MADCTL);
    if (rotation == 0) {
        writeData(0x00);
    } else if (rotation == 1) {
        writeData(0xC0);
    }
}

ST7735S_Display::ST7735S_Display(uint8_t cs, uint8_t rst, uint8_t dc, uint8_t bl, uint8_t sck, uint8_t mosi) 
    : _cs(cs), _rst(rst), _dc(dc), _bl(bl), _sck(sck), _mosi(mosi) {}

void ST7735S_Display::begin() {
    SPI.begin(_sck, -1, _mosi, _cs);
    SPI.setFrequency(40000000);
    pinMode(_cs, OUTPUT);
    pinMode(_rst, OUTPUT);
    pinMode(_dc, OUTPUT);
    pinMode(_bl, OUTPUT);

    digitalWrite(_bl, HIGH); // Encender retroiluminación
    
    // Resetear la pantalla
    digitalWrite(_rst, LOW);
    delay(50);
    digitalWrite(_rst, HIGH);
    delay(50);
    
    // Inicializar la pantalla
    writeCommand(ST7735_SWRESET);
    delay(150);
    writeCommand(ST7735_SLPOUT);
    delay(500);
    writeCommand(ST7735_COLMOD);
    writeData(0x05); // 16-bit color
    delay(10);
    writeCommand(ST7735_DISPON);
    delay(100);
    setRotation(1); // Rotar 180°
}


void ST7735S_Display::updateTextArea(uint8_t x, uint8_t y, const char* oldText, 
  const char* newText, uint8_t scale, 
  uint16_t bgColor, uint16_t textColor) {
if(strcmp(oldText, newText) != 0) {
  // 1. Borrar texto anterior
  drawTextRotated(oldText, x, y, bgColor, scale);

  // 2. Dibujar nuevo texto
  drawTextRotated(newText, x, y, textColor, scale);
  }
}

void ST7735S_Display::setCustomLabel(const char* newLabel) {
  strncpy(customLabel, newLabel, sizeof(customLabel) - 1);
  customLabel[sizeof(customLabel) - 1] = '\0'; // Asegurar terminación nula
}

void ST7735S_Display::updateDisplay(float v1, float v2, float v3) {
  const uint8_t scale = 1.9;
  const uint16_t textColor = 0xFFFF;
  const uint16_t bgColor = 0x0000;
  
  // Buffer para nuevos valores
  char newText4[10], newText5[10], newText6[10];
  snprintf(newText4, sizeof(newText4), "%.1f", v1);
  snprintf(newText5, sizeof(newText5), "%.1f", v2);
  snprintf(newText6, sizeof(newText6), "%.1f", v3);

  // Dibujar etiquetas solo la primera vez
  if(firstUpdate) {
      drawTextRotated(customLabel, 100, 60, 0xFFFF, scale);
      drawTextRotated("Posicion_Actual:", 60, 60, 0xFFFF, scale);
      drawTextRotated("Tiempo_Meta:", 20, 70, 0xFFFF, scale);
      firstUpdate = false;
  }

  // Actualizar solo valores modificados
  updateTextArea(100, 130, lastText4, newText4, scale, bgColor, textColor);
  updateTextArea(60, 130, lastText5, newText5, scale, bgColor, textColor);
  updateTextArea(20, 130, lastText6, newText6, scale, bgColor, textColor);

  // Guardar últimos valores
  strcpy(lastText4, newText4);
  strcpy(lastText5, newText5);
  strcpy(lastText6, newText6);

}
