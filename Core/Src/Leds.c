#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Leds.h"

static COLOR_RGB gTemperatures[] = {{255, 255, 255}, {147, 255, 41}, {241, 255, 224}, {235, 212, 255}, {244, 255, 242}};
#ifdef TAK
static COLOR_RGB gColorCorrection = {255, 255, 255};
#define BRIGHTNESS 255
#define PRINT printf
#define BUFFER_DUMP BufferDump
#else
static COLOR_RGB gColorCorrection = {176, 255, 240};
#define BRIGHTNESS 50
#define PRINT
#define BUFFER_DUMP
#endif

typedef struct
{
  uint16_t *Buffer;
  LED_SECTION *Sections;
  uint8_t SectionsSize;
  uint16_t BufferIndex;
  uint16_t BufferSize;
  uint8_t LedIndex;
  uint8_t SectionIndex;
} LED_CONFIG;

LED_CONFIG *gConfigs = NULL;
uint8_t gConfigsSize = 0;
uint8_t gConfigNumber = 0;
uint8_t gHue = 0;

#ifdef TAK

void BufferDump(uint8_t ConfigIndex) {
  PRINT("Buffer dump:\n");
  for (int i = 0; i < gConfigs[ConfigIndex].BufferSize / 24; i++) {
    PRINT("%1d [", i);
    for (int j = 0; j < 24; j++) {
      PRINT("%d:%1d ", j, gConfigs[ConfigIndex].Buffer[i * 24 + j] );
    }
    PRINT("\b]\n");
  }
  PRINT("\n");
}

#endif


void InitializeConfigs(uint8_t AmountOfConfigs) {
  if (gConfigs != NULL) {
    return;
  }

  gConfigs = malloc(sizeof(LED_CONFIG) * AmountOfConfigs);
  memset(gConfigs, 0, sizeof(LED_CONFIG) * AmountOfConfigs);
  gConfigsSize = AmountOfConfigs;
}

uint8_t InitializeConfig(uint8_t ConfigIndex, uint8_t AmountOfSections, const uint8_t *LedCounts, uint16_t *Buffer, uint16_t BufferSize) {
  if (gConfigNumber >= gConfigsSize) {
    return 1;
  }

  if (gConfigs[ConfigIndex].Sections != NULL) {
    return 2;
  }

  gConfigs[ConfigIndex].Sections = malloc(sizeof(LED_SECTION) * AmountOfSections);
  memset(gConfigs[ConfigIndex].Sections, 0, sizeof(LED_SECTION) * AmountOfSections);
  for (int Index = 0; Index < AmountOfSections; Index++) {
    gConfigs[ConfigIndex].Sections[Index].LedCount = LedCounts[Index];
  }
  gConfigs[ConfigIndex].SectionsSize = AmountOfSections;
  gConfigs[ConfigIndex].BufferSize = BufferSize;
  gConfigs[ConfigIndex].Buffer = Buffer;
  gConfigNumber++;
  return 0;
}

LED_SECTION *GetLedSection(const uint8_t ConfigIndex, const uint8_t SectionIndex) {
  return &gConfigs[ConfigIndex].Sections[SectionIndex];
}

uint8_t GetPwmValueFromColor(const uint8_t ConfigIndex) {
#ifdef TAK
  static uint8_t PwmValue[2] = {0, 1};
#else
  static uint8_t PwmValue[2] = {30, 60};
#endif
  uint8_t ColorOffset = (gConfigs[ConfigIndex].BufferIndex % 24) / 8;
  uint8_t ColorBitOffset = 7 - (gConfigs[ConfigIndex].BufferIndex % 8);
  uint8_t *TempPointer = (uint8_t *)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]));
  uint8_t TemperatureValue = (*(uint8_t *)(TempPointer + ColorOffset));
  TempPointer = (uint8_t *)(&gColorCorrection);
  uint8_t CorrectionValue = (*(uint8_t *)(TempPointer + ColorOffset));
  uint32_t ColorByte = *(((uint8_t *)&gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].Color) + ColorOffset);
  ColorByte = (ColorByte * (uint32_t)(TemperatureValue * BRIGHTNESS * CorrectionValue));
  ColorByte = ColorByte / ((uint32_t)(255 * 255 * 255));
  return PwmValue[(ColorByte >> ColorBitOffset) & 1];
}

uint8_t FillHalfBuffer(const uint8_t ConfigIndex) {
  uint16_t BufferFilled = 0;
  uint16_t BufferToBeFilled = gConfigs[ConfigIndex].BufferSize / 2;
  uint16_t BufferSizeIteration = gConfigs[ConfigIndex].BufferIndex;
  uint8_t ReturnValue = 0;

  if (gConfigs[ConfigIndex].BufferIndex != BufferToBeFilled && gConfigs[ConfigIndex].BufferIndex != 0) {
    return 2;
  }

  for (; gConfigs[ConfigIndex].SectionIndex < gConfigs[ConfigIndex].SectionsSize; gConfigs[ConfigIndex].SectionIndex++) {
    for (; gConfigs[ConfigIndex].LedIndex < gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].LedCount; gConfigs[ConfigIndex].LedIndex++) {
      if (BufferFilled == BufferToBeFilled) {
        goto end;
      }

      BufferSizeIteration += 24;
      for (; gConfigs[ConfigIndex].BufferIndex < BufferSizeIteration; gConfigs[ConfigIndex].BufferIndex++, BufferFilled++) {
        gConfigs[ConfigIndex].Buffer[gConfigs[ConfigIndex].BufferIndex] = GetPwmValueFromColor(ConfigIndex);
      }
    }
    if (BufferFilled == BufferToBeFilled) {
      goto end;
    }

    gConfigs[ConfigIndex].LedIndex = 0;
  }

  if (BufferFilled != BufferToBeFilled) {
    for (; BufferFilled < BufferToBeFilled; BufferFilled++, gConfigs[ConfigIndex].BufferIndex++) {
      gConfigs[ConfigIndex].Buffer[gConfigs[ConfigIndex].BufferIndex] = 0;      
    }

    ReturnValue = 1;
  }

end:
  gConfigs[ConfigIndex].BufferIndex %= gConfigs[ConfigIndex].BufferSize;
  return ReturnValue;
}

uint8_t PrepareBufferForTransaction(uint8_t ConfigIndex) {
  uint8_t ReturnValue = 0;

  gConfigs[ConfigIndex].LedIndex = 0;
  gConfigs[ConfigIndex].SectionIndex = 0;
  gConfigs[ConfigIndex].BufferIndex = 0;
  ReturnValue = FillHalfBuffer(ConfigIndex);
  ReturnValue = FillHalfBuffer(ConfigIndex);
  return ReturnValue;
}

void ShowEffectRainbow(uint8_t ConfigIndex, uint8_t ColorStep, uint8_t HueStep) {
  uint8_t SectionIndex;
    for (SectionIndex = 0; SectionIndex < gConfigs[ConfigIndex].SectionsSize; SectionIndex++) {
      gConfigs[ConfigIndex].Sections[SectionIndex].Color = HsvToRgb ((COLOR_HSV){gHue + SectionIndex * ColorStep, 255, 255});
    }
    gHue += HueStep;
    PrepareBufferForTransaction(0);
}

void ShowEffectFade(uint8_t ConfigIndex, uint8_t Step) {
  COLOR_HSV HsvColor;
  uint8_t SectionIndex;
  for (SectionIndex = 0; SectionIndex < gConfigs[ConfigIndex].SectionsSize; SectionIndex++) {
    HsvColor = RgbToHsv (gConfigs[ConfigIndex].Sections[SectionIndex].Color);
    if (HsvColor.v > Step) {
      HsvColor.v -= Step;
    } else if (HsvColor.v > 0) {
      HsvColor.v -= HsvColor.v;
    }
    gConfigs[ConfigIndex].Sections[SectionIndex].Color = HsvToRgb(HsvColor);
  }
  PrepareBufferForTransaction(0);
}

COLOR_RGB HsvToRgb(COLOR_HSV hsv) {
  COLOR_RGB rgb;
  unsigned char region, remainder, p, q, t;

  if (hsv.s == 0) {
      rgb.Red = hsv.v;
      rgb.Green = hsv.v;
      rgb.Blue = hsv.v;
      return rgb;
  }

  region = hsv.h / 43;
  remainder = (hsv.h - (region * 43)) * 6;

  p = (hsv.v * (255 - hsv.s)) >> 8;
  q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
  t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
      case 0:
          rgb.Red = hsv.v; rgb.Green = t; rgb.Blue = p;
          break;
      case 1:
          rgb.Red = q; rgb.Green = hsv.v; rgb.Blue = p;
          break;
      case 2:
          rgb.Red = p; rgb.Green = hsv.v; rgb.Blue = t;
          break;
      case 3:
          rgb.Red = p; rgb.Green = q; rgb.Blue = hsv.v;
          break;
      case 4:
          rgb.Red = t; rgb.Green = p; rgb.Blue = hsv.v;
          break;
      default:
          rgb.Red = hsv.v; rgb.Green = p; rgb.Blue = q;
          break;
  }

  return rgb;
}

COLOR_HSV RgbToHsv(COLOR_RGB rgb) {
  COLOR_HSV hsv;
  uint8_t rgbMin, rgbMax;

  rgbMin = rgb.Red < rgb.Green ? (rgb.Red < rgb.Blue ? rgb.Red : rgb.Blue) : (rgb.Green < rgb.Blue ? rgb.Green : rgb.Blue);
  rgbMax = rgb.Red > rgb.Green ? (rgb.Red > rgb.Blue ? rgb.Red : rgb.Blue) : (rgb.Green > rgb.Blue ? rgb.Green : rgb.Blue);

  hsv.v = rgbMax;
  if (hsv.v == 0) {
      hsv.h = 0;
      hsv.s = 0;
      return hsv;
  }

  hsv.s = 255 * (int32_t)(rgbMax - rgbMin) / hsv.v;
  if (hsv.s == 0) {
      hsv.h = 0;
      return hsv;
  }

  if (rgbMax == rgb.Red) {
    hsv.h = 0 + 43 * (rgb.Green - rgb.Blue) / (rgbMax - rgbMin);
  } else if (rgbMax == rgb.Green) {
    hsv.h = 85 + 43 * (rgb.Blue - rgb.Red) / (rgbMax - rgbMin);
  } else {
    hsv.h = 171 + 43 * (rgb.Red - rgb.Green) / (rgbMax - rgbMin);
  }

  return hsv;
}

#ifdef TAK

void CheckValues(uint8_t LedsAmount, COLOR_RGB* LedColors, uint16_t* Buffer) {
  uint8_t LedIndex;
  int8_t BitIndex;
  uint8_t Index;
  const uint8_t ColorTable[] = {0, 1};
  uint8_t ExpectedColor;
  uint8_t ActualColor;
  uint8_t ExpectedBufferIndex;
  uint8_t ActualBufferIndex;
  uint8_t *Colors = (uint8_t*)(LedColors);
  uint8_t Failed = 0;

  for (LedIndex = 0; LedIndex < LedsAmount; LedIndex++) {
    for (Index = 0; Index < 3; Index++) {
      for (BitIndex = 7; BitIndex >= 0; BitIndex--) {
        ExpectedBufferIndex = LedIndex * 3 + Index;
        ActualBufferIndex = (LedIndex * 3 * 8) + (Index * 8) + BitIndex;
        ExpectedColor = ColorTable[((Colors[ExpectedBufferIndex] >> (7 - BitIndex)) & 0x1)];
        ActualColor = Buffer[ActualBufferIndex];
        if (ExpectedColor != ActualColor) {
          Failed = 1;
          PRINT("Led[%d] Color[%d] Bit[%d] incorrect should be %d is %d Actual %d Expected %d\n", LedIndex, Index, BitIndex, ExpectedColor , ActualColor, ActualBufferIndex, ExpectedBufferIndex);
        }
      }
    }
  }
  assert(Failed == 0);
}

int main() {
  uint16_t Buffer1[24 * 8];
  InitializeConfigs(1);
  uint8_t leds[] = {10, 12, 14, 16};
  InitializeConfig(0, 4, leds, Buffer1, 8 * 24);
  COLOR_RGB tak = {150, 160, 170};
  COLOR_RGB tak1 = {150, 160, 170};
  COLOR_RGB tak2 = {10, 20, 30};
  COLOR_RGB tak3 = {12, 18, 15};
  COLOR_RGB zero = {0, 0, 0};
  GetLedSection(0, 0)->Color = tak;
  GetLedSection(0, 1)->Color = tak1;
  GetLedSection(0, 2)->Color = tak2;
  GetLedSection(0, 3)->Color = tak3;
  PrepareBufferForTransaction(0);
  COLOR_RGB LedColors1[8] = {tak, tak, tak, tak, tak, tak, tak, tak};
  CheckValues (8, LedColors1, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors2[8] = {tak, tak, tak1, tak1, tak, tak, tak, tak};
  CheckValues (8, LedColors2, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors3[8] = {tak, tak, tak1, tak1, tak1, tak1, tak1, tak1};
  CheckValues (8, LedColors3, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors4[8] = {tak1, tak1, tak1, tak1, tak1, tak1, tak1, tak1};
  CheckValues (8, LedColors4, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors5[8] = {tak1, tak1, tak1, tak1, tak1, tak1, tak2, tak2};
  CheckValues (8, LedColors5, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors6[8] = {tak2, tak2, tak2, tak2, tak1, tak1, tak2, tak2};
  CheckValues (8, LedColors6, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors7[8] = {tak2, tak2, tak2, tak2, tak2, tak2, tak2, tak2};
  CheckValues (8, LedColors7, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors8[8] = {tak2, tak2, tak2, tak2, tak2, tak2, tak2, tak2};
  CheckValues (8, LedColors8, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors9[8] = {tak2, tak2, tak2, tak2, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors9, Buffer1);

  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors10[8] = {tak3, tak3, tak3, tak3, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors10, Buffer1);
  PRINT("End value: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors11[8] = {tak3, tak3, tak3, tak3, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors11, Buffer1);

  PRINT("End value 12: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors12[8] = {tak3, tak3, tak3, tak3, tak3, tak3, tak3, tak3};
  CheckValues (8, LedColors12, Buffer1);
  PRINT("End value 13: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors13[8] = {tak3, tak3, tak3, tak3, zero, zero, zero, zero};
  CheckValues (8, LedColors13, Buffer1);
  PRINT("End value 14: %d\n", FillHalfBuffer(0));
  COLOR_RGB LedColors14[8] = {zero, zero, zero, zero, zero, zero, zero, zero};
  CheckValues (8, LedColors14, Buffer1);
}

#endif
