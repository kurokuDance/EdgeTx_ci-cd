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

#pragma once

#if !defined(BOOT)
#include "datastructs_private.h"

#if !defined(BACKUP)
/* Compile time check to test structure size has not changed *
   Changing the size of one of the eeprom structs may cause wrong data to
   be loaded. Error out if the struct size changes.
   This function tries not avoid checking or using the defines
   other than the CPU arch and board type so changes in other
   defines also trigger the struct size changes */

#include "chksize.h"

#define CHKSIZE(x, y) check_size<struct x, y>()
#define CHKTYPE(x, y) check_size<x, y>()

static inline void check_struct()
{

  CHKSIZE(CurveRef, 2);

  /* Difference between Taranis/Horus is LEN_EXPOMIX_NAME */
  /* LEN_xFUNCTION_NAME is the difference in CustomFunctionData */

  CHKSIZE(VarioData, 5);

#if defined(PCBX7) || defined(PCBXLITE) || defined(PCBX9LITE)
  CHKSIZE(MixData, 20);
  CHKSIZE(ExpoData, 17);
  CHKSIZE(LimitData, 11);
  CHKSIZE(LogicalSwitchData, 9);
  CHKSIZE(CustomFunctionData, 11);
  CHKSIZE(FlightModeData, 28 + 2 * MAX_TRIMS);
  CHKSIZE(TimerData, 12);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(FrSkyBarData, 6);
  CHKSIZE(FrSkyLineData, 4);
  CHKTYPE(TelemetryScreenData, 24);
  CHKSIZE(ModelHeader, 12);
  CHKSIZE(CurveHeader, 4);
#elif defined(PCBTARANIS)
  CHKSIZE(MixData, 20);
  CHKSIZE(ExpoData, 17);
  CHKSIZE(LimitData, 13);
  CHKSIZE(LogicalSwitchData, 9);
  CHKSIZE(CustomFunctionData, 11);
  CHKSIZE(FlightModeData, 44);
  CHKSIZE(TimerData, 17);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(FrSkyBarData, 6);
  CHKSIZE(FrSkyLineData, 6);
  CHKTYPE(TelemetryScreenData, 24);
  CHKSIZE(ModelHeader, 24);
  CHKSIZE(CurveHeader, 4);
#elif defined(PCBHORUS)
  CHKSIZE(MixData, 20);
  CHKSIZE(ExpoData, 17);
  CHKSIZE(LimitData, 13);
  CHKSIZE(CustomFunctionData, 9);
  CHKSIZE(FlightModeData, 44);
  CHKSIZE(TimerData, 17);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(ModelHeader, 131);
  CHKSIZE(CurveHeader, 4);
  CHKSIZE(CustomScreenData, 852);
  CHKTYPE(TopBarPersistentData, 444);
#elif defined(PCBNV14) || defined(PCBPL18)
  // TODO
#else
  // Common for all variants
  CHKSIZE(LimitData, 5);
  CHKSIZE(SwashRingData, 3);
  CHKSIZE(FrSkyBarData, 3);
  CHKSIZE(FrSkyLineData, 2);
  CHKSIZE(ModelHeader, 11);
  CHKTYPE(CurveHeader, 1);

  CHKSIZE(MixData, 9);
  CHKSIZE(ExpoData, 4);

  CHKSIZE(CustomFunctionData, 3);
  CHKSIZE(TimerData, 3);

  CHKSIZE(FlightModeData, 30);
  CHKSIZE(RadioData, 86);

#endif /* board specific ifdefs*/

  CHKSIZE(LogicalSwitchData, 9);
  CHKSIZE(TelemetrySensor, 14);
  CHKSIZE(ModuleData, 29);
  CHKSIZE(GVarData, 7);
  CHKSIZE(RFAlarmData, 2);
  CHKSIZE(TrainerData, 16);

#if defined(PCBXLITES)
  CHKSIZE(RadioData, 871);
  CHKSIZE(ModelData, 6265);
#elif defined(PCBXLITE)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6265);
#elif defined(RADIO_TPRO)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6290);
#elif defined(RADIO_POCKET)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6265);
#elif defined(RADIO_TPROV2)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6290);
#elif defined(RADIO_T14)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6265);
#elif defined(RADIO_FAMILY_T20)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6326);
#elif defined(RADIO_BOXER)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6265);
#elif defined(RADIO_MT12)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6265);
#elif defined(PCBX7)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6265);
#elif defined(PCBX9E)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6707);
#elif defined(PCBX9D) || defined(PCBX9DP)
  CHKSIZE(RadioData, 869);
  CHKSIZE(ModelData, 6706);
#elif defined(PCBHORUS)
  #if defined(PCBX10)
    CHKSIZE(RadioData, 837);
    CHKSIZE(ModelData, 15607);
  #else
    CHKSIZE(RadioData, 837);
    CHKSIZE(ModelData, 15607);
  #endif
#elif defined(PCBNV14)
  CHKSIZE(RadioData, 837);
  CHKSIZE(ModelData, 15463);
#endif

#undef CHKSIZE
}
#endif /* BACKUP */
#endif /* !BOOT */
