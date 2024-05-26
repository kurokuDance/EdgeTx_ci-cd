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

#include "timer_setup.h"

#include "opentx.h"
#include "switchchoice.h"
#include "timeedit.h"

#define SET_DIRTY() storageDirty(EE_MODEL)

static const lv_coord_t line_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                          LV_GRID_TEMPLATE_LAST};
static const lv_coord_t line_row_dsc[] = {LV_GRID_CONTENT,
                                          LV_GRID_TEMPLATE_LAST};

static std::function<void(uint32_t)> timerValueUpdater(uint8_t timer)
{
  return [=](uint32_t value) {
    TimerData* p_timer = &g_model.timers[timer];
    p_timer->start = value;
    timerSet(timer, value);
    SET_DIRTY();
  };
}

static void timer_start_changed(lv_event_t* e)
{
  lv_obj_t* target = lv_event_get_target(e);
  auto obj = (lv_obj_t*)lv_event_get_user_data(e);
  auto val = (TimeEdit*)lv_obj_get_user_data(target);

  if (!obj || !val) return;

  if (val->getValue() > 0) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

TimerWindow::TimerWindow(uint8_t timer) : Page(ICON_STATS_TIMERS)
{
  std::string title2 = std::string(STR_TIMER) + std::to_string(timer + 1);
  header->setTitle(STR_MENU_MODEL_SETUP);
  header->setTitle2(title2);

  body->setFlexLayout();

  FlexGridLayout grid(line_col_dsc, line_row_dsc, PAD_TINY);

  TimerData* p_timer = &g_model.timers[timer];

  // Timer name
  auto line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_NAME);
  new ModelTextEdit(line, rect_t{}, p_timer->name, LEN_TIMER_NAME);

  // Timer mode
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_MODE);
  new Choice(line, rect_t{}, STR_TIMER_MODES, 0, TMRMODE_MAX,
             GET_SET_DEFAULT(p_timer->mode));

  // Timer switch
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_SWITCH);
  new SwitchChoice(line, rect_t{}, SWSRC_FIRST, SWSRC_LAST,
                   GET_SET_DEFAULT(p_timer->swtch));

  // Timer start value
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_START);
  auto timerValue =
      new TimeEdit(line, rect_t{}, 0, TIMER_MAX,
                   GET_DEFAULT(p_timer->start), timerValueUpdater(timer));
  timerValue->setAccelFactor(16);

  // Timer direction
  auto timerDirLine = body->newLine(grid);
  new StaticText(timerDirLine, rect_t{}, STR_LIMITS_HEADERS_DIRECTION);
  new Choice(timerDirLine, rect_t{}, STR_TIMER_DIR, 0, 1,
             GET_SET_DEFAULT(p_timer->showElapsed));

  if (timerValue->getValue() == 0) {
    timerDirLine->hide();
  }

  // Timer minute beep
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_MINUTEBEEP);
  new ToggleSwitch(line, rect_t{}, GET_SET_DEFAULT(p_timer->minuteBeep));

  // Timer countdown
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_BEEPCOUNTDOWN);

  auto box = new Window(line, rect_t{});
  box->padAll(PAD_TINY);
  box->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_TINY, LV_SIZE_CONTENT);

  new Choice(
      box, rect_t{}, STR_VBEEPCOUNTDOWN, COUNTDOWN_SILENT, COUNTDOWN_COUNT - 1,
      [=]() -> int {
        int value = p_timer->countdownBeep;
        if (p_timer->extraHaptic) {
          value += (COUNTDOWN_NON_HAPTIC_LAST + 1);
        }
        return (value);
      },
      [=](int value) {
        if (value > COUNTDOWN_NON_HAPTIC_LAST + 1) {
          p_timer->extraHaptic = 1;
          p_timer->countdownBeep = value - (COUNTDOWN_NON_HAPTIC_LAST + 1);
        } else {
          p_timer->extraHaptic = 0;
          p_timer->countdownBeep = value;
        }
        SET_DIRTY();
        TRACE("value=%d\tcountdownBeep = %d\textraHaptic = %d", value,
              p_timer->countdownBeep, p_timer->extraHaptic);
      });

  new Choice(box, rect_t{}, STR_COUNTDOWNVALUES, 0, 3,
             GET_SET_WITH_OFFSET(p_timer->countdownStart, 2));

  // Timer persistent
  line = body->newLine(grid);
  new StaticText(line, rect_t{}, STR_PERSISTENT);

  new Choice(line, rect_t{}, STR_VPERSISTENT, 0, 2,
             GET_SET_DEFAULT(p_timer->persistent));

  lv_obj_add_event_cb(timerValue->getLvObj(), timer_start_changed,
                      LV_EVENT_VALUE_CHANGED, timerDirLine->getLvObj());
}
