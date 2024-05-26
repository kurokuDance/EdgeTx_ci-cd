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

#include "page.h"
#include "pulses/afhds3_config.h"

struct PWMfrequencyChoice : public Window {
  PWMfrequencyChoice(Window* parent, uint8_t moduleIdx, uint8_t channelIdx);
  PWMfrequencyChoice(Window* parent, uint8_t moduleIdx);
  void update() const;

 private:
  lv_obj_t* c_obj;
};

class AFHDS3_Options : public Page
{
  afhds3::Config_u* cfg;

 public:
  AFHDS3_Options(uint8_t moduleIdx);
};
