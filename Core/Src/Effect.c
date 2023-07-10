#include <stdint.h>

#include "Common.h"
#include "Effect.h"

static PALLETE gBasicPallete[] = {{0, {255, 0, 0}}, {40, {0, 255, 0}}, {255, {255, 0, 0}}};
static PALLETE_ARRAY gPalletes[] = {{gBasicPallete, LENGTH_OF (gBasicPallete)}};

COLOR_GRB GetColorFromPalleteSmooth (uint8_t Angle, uint8_t PalleteArrayIndex) {
  uint8_t Index;
  uint8_t Found = 0;
  PALLETE *Pallete = gPalletes[PalleteArrayIndex].Pallete;
  for (Index = 0; Index < gPalletes[PalleteArrayIndex].Length - 1; Index++) {
    if (Angle >= Pallete[Index].Angle && Angle < Pallete[Index + 1].Angle) {
      Found = 1;
      break;
    }
  }

  if (Found) {
    uint8_t a = RgbToHsv(Pallete[Index].Rgb).h;
    uint8_t b = RgbToHsv(Pallete[Index+1].Rgb).h;
    uint8_t t = (Angle - Pallete[Index].Angle) * (255 / (Pallete[Index + 1].Angle - Pallete[Index].Angle));
    return HsvToRgb((COLOR_HSV){LerpHSV (a, b, t), 255, 255});
  } else {
    return Pallete[Index].Rgb;
  }
}

COLOR_GRB GetColorFromPalleteSolid (uint8_t Angle, uint8_t PalleteArrayIndex) {
  uint8_t Index;
  PALLETE *Pallete = gPalletes[PalleteArrayIndex].Pallete;
  for (Index = 0; Index < gPalletes[PalleteArrayIndex].Length - 1; Index++) {
    if (Angle >= Pallete[Index].Angle && Angle < Pallete[Index + 1].Angle) {
      break;
    }
  }

  return Pallete[Index].Rgb;
}

uint8_t LerpHSV (uint8_t a, uint8_t b, uint8_t t) {
  float tempa = (float)(a) / 255;
  float tempt = (float)(t) / 255;
  float tempb = (float)(b) / 255;

  float tempd = (float)(b - a) / 255;
  float h;
  uint8_t hue;

  if (tempa > tempb) {
    float temp = tempa;
    tempa = tempb;
    tempb = temp;
    tempd = -tempd;
    tempt = 1 - tempt;
  }

  if (tempd > 0.5) {
    tempa = tempa + 1;
    h = (tempa + tempt * (tempb - tempa)); // 360deg
    while (h > 1.0) {
      h -= 1.0;
    }
    while (h < -1.0) {
      h += 1.0;
    }
  } else if (tempd <= 0.5) {
    h = tempa + tempt * tempd;
  }
   hue = h * 255;
  return hue;
}
