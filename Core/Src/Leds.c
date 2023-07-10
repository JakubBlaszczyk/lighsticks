#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Leds.h"

static COLOR_RGB gTemperatures[] = {{255, 255, 255}, {147, 255, 41}, {241, 255, 224}, {235, 212, 255}, {244, 255, 242}};
static COLOR_RGB gColorCorrection = {176, 255, 240};

typedef struct
{
  uint16_t *Buffer;
  LED_SECTION *Sections;
  uint8_t SectionsSize;
  uint16_t BufferIndex;
  uint16_t BufferSize;
  uint8_t Brightness;
  uint8_t LedIndex;
  uint8_t SectionIndex;
} LED_CONFIG;

LED_CONFIG *gConfigs = NULL;
uint8_t gConfigsSize = 0;
uint8_t gConfigNumber = 0;
uint8_t gHue = 0;

void InitializeConfigs(uint8_t AmountOfConfigs) {
  if (gConfigs != NULL) {
    return;
  }

  gConfigs = malloc(sizeof(LED_CONFIG) * AmountOfConfigs);
  memset(gConfigs, 0, sizeof(LED_CONFIG) * AmountOfConfigs);
  gConfigsSize = AmountOfConfigs;
}

uint8_t InitializeConfig(uint8_t ConfigIndex, uint8_t AmountOfSections, const uint8_t *LedCounts, uint16_t *Buffer, uint16_t BufferSize) {
  uint8_t Index;

  if (gConfigNumber >= gConfigsSize) {
    return 1;
  }

  if (gConfigs[ConfigIndex].Sections != NULL) {
    return 2;
  }

  gConfigs[ConfigIndex].Sections = malloc(sizeof(LED_SECTION) * AmountOfSections);
  memset(gConfigs[ConfigIndex].Sections, 0, sizeof(LED_SECTION) * AmountOfSections);

  for (Index = 0; Index < AmountOfSections; Index++) {
    if (LedCounts == NULL) {
      gConfigs[ConfigIndex].Sections[Index].LedCount = 1;
    } else {
      gConfigs[ConfigIndex].Sections[Index].LedCount = LedCounts[Index];
    }
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

static uint8_t GetPwmValueFromColor(const uint8_t ConfigIndex) {
  static uint8_t PwmValue[2] = {27, 53};
  uint8_t ColorOffset = (gConfigs[ConfigIndex].BufferIndex % 24) / 8;
  uint8_t ColorBitOffset = 7 - (gConfigs[ConfigIndex].BufferIndex % 8);
  uint8_t *TempPointer = (uint8_t *)(&(gTemperatures[gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].TemperatureIndex]));
  uint8_t TemperatureValue = (*(uint8_t *)(TempPointer + ColorOffset));
  TempPointer = (uint8_t *)(&gColorCorrection);
  uint8_t CorrectionValue = (*(uint8_t *)(TempPointer + ColorOffset));
  uint32_t ColorByte = *(((uint8_t *)&gConfigs[ConfigIndex].Sections[gConfigs[ConfigIndex].SectionIndex].Color) + ColorOffset);
  ColorByte = (ColorByte * (uint32_t)(TemperatureValue * gConfigs[ConfigIndex].Brightness * CorrectionValue));
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

void ShowEffectHueue(uint8_t ConfigIndex, uint8_t ColorStep, uint8_t HueStep) {
  uint8_t SectionIndex;
    for (SectionIndex = 0; SectionIndex < gConfigs[ConfigIndex].SectionsSize; SectionIndex++) {
      gConfigs[ConfigIndex].Sections[SectionIndex].Color = HsvToRgb ((COLOR_HSV){gHue + SectionIndex * ColorStep, 255, 255});
    }
    gHue += HueStep;
}

void ShowEffectRainbow(uint8_t ConfigIndex, uint8_t ColorStep, uint8_t HueStep) {
  uint8_t SectionIndex;
    for (SectionIndex = 0; SectionIndex < gConfigs[ConfigIndex].SectionsSize; SectionIndex++) {
      gConfigs[ConfigIndex].Sections[SectionIndex].Color = HsvToRgb ((COLOR_HSV){gHue + SectionIndex * ColorStep, 255, 255});
    }
    gHue += HueStep;
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
