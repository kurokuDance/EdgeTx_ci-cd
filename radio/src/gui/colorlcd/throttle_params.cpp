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

#include "throttle_params.h"
#include "opentx.h"
#include "sourcechoice.h"
#include "switchchoice.h"

#define SET_DIRTY()     storageDirty(EE_MODEL)

static const lv_coord_t line_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                          LV_GRID_TEMPLATE_LAST};
static const lv_coord_t line_row_dsc[] = {LV_GRID_CONTENT,
                                          LV_GRID_TEMPLATE_LAST};

static int16_t getThrottleSource()
{
  return throttleSource2Source(g_model.thrTraceSrc);
}

static void setThrottleSource(int16_t src)
{
  int16_t val = source2ThrottleSource(src);
  if (val >= 0) {
    g_model.thrTraceSrc = val;
    SET_DIRTY();
  }
}

static int16_t getThrottleTrimSource()
{
  return g_model.getThrottleStickTrimSource();
}

static void setThrottleTrimSource(int16_t src)
{
  g_model.setThrottleStickTrimSource(src);
}

ThrottleParams::ThrottleParams() : Page(ICON_MODEL_SETUP)
{
  header->setTitle(STR_MENU_MODEL_SETUP);
  header->setTitle2(STR_THROTTLE_LABEL);

  body->setFlexLayout();
  FlexGridLayout grid(line_col_dsc, line_row_dsc);

  // Throttle reversed
  auto line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_THROTTLEREVERSE);
  new ToggleSwitch(line, rect_t{}, GET_SET_DEFAULT(g_model.throttleReversed));

  // Throttle source
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_TTRACE);
  auto sc = new SourceChoice(line, rect_t{}, 0, MIXSRC_LAST_CH,
                             getThrottleSource, setThrottleSource);
  sc->setAvailableHandler(isThrottleSourceAvailable);

  // Throttle trim
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_TTRIM);
  new ToggleSwitch(line, rect_t{}, GET_SET_DEFAULT(g_model.thrTrim));

  // Throttle trim source
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_TTRIM_SW);
  new SourceChoice(
      line, rect_t{}, MIXSRC_FIRST_TRIM, MIXSRC_LAST_TRIM,
      getThrottleTrimSource, setThrottleTrimSource);
}
