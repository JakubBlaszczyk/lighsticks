
#ifndef _EFFECT_
#define _EFFECT_

#include "Common.h"

typedef struct {
  uint8_t Angle;
  COLOR_GRB Rgb;
} PALLETE;

typedef struct {
  PALLETE *Pallete;
  uint8_t Length;
} PALLETE_ARRAY;

COLOR_GRB GetColorFromPallete (uint8_t Angle, PALLETE *Pallete, uint8_t PalleteSize);
uint8_t LerpHSV (uint8_t a, uint8_t b, uint8_t t);


#endif // _EFFECT_
