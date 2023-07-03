#ifndef _LEDS_
#define _LEDS_

typedef struct
{
  uint8_t Green;
  uint8_t Red;
  uint8_t Blue;
} COLOR_RGB;

typedef struct
{
  uint8_t LedCount;
  COLOR_RGB Color;
  uint8_t TemperatureIndex;
} LED_SECTION;

typedef struct COLOR_HSV
{
  uint8_t h;
  uint8_t s;
  uint8_t v;
} COLOR_HSV;

#define TEMPERATURE_UNCORRECTED 0
#define TEMPERATURE_CANDLE 1
#define TEMPERATURE_HALOGEN 2
#define TEMPERATURE_COOLWHITEFLUORESCENT 3
#define TEMPERATURE_FULLSPECTRUMFLUORESCENT 4

void InitializeConfigs(uint8_t AmountOfConfigs);
uint8_t InitializeConfig(uint8_t ConfigIndex, uint8_t AmountOfSections, const uint8_t *LedCounts, uint16_t *Buffer, uint16_t BufferSize);
LED_SECTION *GetLedSection(const uint8_t ConfigIndex, const uint8_t SectionIndex);
uint8_t FillHalfBuffer(const uint8_t ConfigIndex);
uint8_t PrepareBufferForTransaction(const uint8_t ConfigIndex);
void ShowEffectRainbow(uint8_t ConfigIndex, uint8_t ColorStep, uint8_t HueStep);
void ShowEffectFade(uint8_t ConfigIndex, uint8_t Step);
COLOR_RGB HsvToRgb(COLOR_HSV hsv);
COLOR_HSV RgbToHsv(COLOR_RGB rgb);

#endif // _LEDS_
