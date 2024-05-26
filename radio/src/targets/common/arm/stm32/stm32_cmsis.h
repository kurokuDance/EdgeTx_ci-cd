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

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(STM32F4)

  // Workaround to prevent the other CMSIS header to be included
  // and fix some missing items in the old one
  #include "thirdparty/CMSIS/Include/cmsis_compiler.h"
  #define __CORE_CM4_H_GENERIC

  #include "CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"
#elif defined(STM32F2)

  // Workaround to prevent the other CMSIS header to be included
  // and fix some missing items in the old one
  #include "thirdparty/CMSIS/Include/cmsis_compiler.h"
  #define __CORE_CM3_H_GENERIC

  #include "CMSIS/Device/ST/STM32F2xx/Include/stm32f2xx.h"
#endif

#if defined(__cplusplus)
}
#endif
