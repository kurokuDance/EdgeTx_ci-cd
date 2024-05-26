/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "sbus.h"
#include "timers_driver.h"

#define SBUS_FRAME_GAP_DELAY_US 500

#define SBUS_START_BYTE        0x0F
#define SBUS_END_BYTE          0x00
#define SBUS_FLAGS_IDX         23
#define SBUS_FRAMELOST_BIT     2
#define SBUS_FAILSAFE_BIT      3

#define SBUS_CH_BITS           11
#define SBUS_CH_MASK           ((1<<SBUS_CH_BITS) - 1)

#define SBUS_CH_CENTER         0x3E0

static int (*_sbusAuxGetByte)(void*, uint8_t*) = nullptr;
static void* _sbusAuxGetByteCtx = nullptr;

void sbusSetAuxGetByte(void* ctx, int (*fct)(void*, uint8_t*))
{
  _sbusAuxGetByte = nullptr;
  _sbusAuxGetByteCtx = ctx;
  _sbusAuxGetByte = fct;
}

int sbusAuxGetByte(uint8_t* byte)
{
  auto _getByte = _sbusAuxGetByte;
  auto _ctx = _sbusAuxGetByteCtx;

  if (_getByte) {
    return _getByte(_ctx, byte);
  }

  return -1;
}

static int (*sbusGetByte)(uint8_t*) = nullptr;

void sbusSetGetByte(int (*fct)(uint8_t*))
{
  sbusGetByte = fct;
}

// Range for pulses (ppm input) is [-512:+512]
void processSbusFrame(uint8_t * sbus, int16_t * pulses, uint32_t size)
{
  if (size != SBUS_FRAME_SIZE || sbus[0] != SBUS_START_BYTE ||
      sbus[SBUS_FRAME_SIZE - 1] != SBUS_END_BYTE) {
    return;  // not a valid SBUS frame
  }
  if ((sbus[SBUS_FLAGS_IDX] & (1 << SBUS_FAILSAFE_BIT)) ||
      (sbus[SBUS_FLAGS_IDX] & (1 << SBUS_FRAMELOST_BIT))) {
    return;  // SBUS invalid frame or failsafe mode
  }

  sbus++; // skip start byte

  uint32_t inputbitsavailable = 0;
  uint32_t inputbits = 0;
  for (uint32_t i=0; i<MAX_TRAINER_CHANNELS; i++) {
    while (inputbitsavailable < SBUS_CH_BITS) {
      inputbits |= *sbus++ << inputbitsavailable;
      inputbitsavailable += 8;
    }
    *pulses++ = ((int32_t) (inputbits & SBUS_CH_MASK) - SBUS_CH_CENTER) * 5 / 8;
    inputbitsavailable -= SBUS_CH_BITS;
    inputbits >>= SBUS_CH_BITS;
  }

  trainerResetTimer();
}

void processSbusInput()
{

  // TODO: place this outside of the function
  static uint8_t SbusIndex = 0;
  static uint32_t SbusTimer;
  static uint8_t SbusFrame[SBUS_FRAME_SIZE];

  uint32_t active = 0;

  // Drain input first (if existing)
  uint8_t rxchar;
  auto _getByte = sbusGetByte;
  while (_getByte && (_getByte(&rxchar) > 0)) {
    active = 1;
    if (SbusIndex > SBUS_FRAME_SIZE - 1) {
      SbusIndex = SBUS_FRAME_SIZE - 1;
    }
    SbusFrame[SbusIndex++] = rxchar;
  }

  // Data has been received
  if (active) {
    SbusTimer = timersGetUsTick();
    return;
  }

  // Check if end-of-frame is detected
  if (SbusIndex) {
    if ((uint32_t)(timersGetUsTick() - SbusTimer) > SBUS_FRAME_GAP_DELAY_US) {
      processSbusFrame(SbusFrame, trainerInput, SbusIndex);
      SbusIndex = 0;
    }
  }
}
