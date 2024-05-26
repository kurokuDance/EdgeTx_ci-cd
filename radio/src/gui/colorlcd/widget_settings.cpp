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

#include "widget_settings.h"

#include "color_picker.h"
#include "libopenui.h"
#include "opentx.h"
#include "sourcechoice.h"
#include "switchchoice.h"
#include "view_main.h"

#define SET_DIRTY() storageDirty(EE_MODEL)

static const rect_t widgetSettingsDialogRect = {
    LCD_W / 10,              // x
    LCD_H / 5,               // y
    LCD_W - 2 * LCD_W / 10,  // width
    LCD_H - 2 * LCD_H / 5    // height
};

static const lv_coord_t line_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                          LV_GRID_TEMPLATE_LAST};
static const lv_coord_t line_row_dsc[] = {LV_GRID_CONTENT,
                                          LV_GRID_TEMPLATE_LAST};

WidgetSettings::WidgetSettings(Window* parent, Widget* w) :
    BaseDialog(ViewMain::instance(), STR_WIDGET_SETTINGS, true), widget(w)
{
  FlexGridLayout grid(line_col_dsc, line_row_dsc);
  form->padAll(PAD_SMALL);
  form->padRow(PAD_ZERO);

  uint8_t optIdx = 0;
  auto optPtr = widget->getOptions();
  while (optPtr && optPtr->name != nullptr) {
    auto line = form->newLine(grid);

    auto option = *optPtr;
    new StaticText(line, rect_t{},
                   option.displayName ? option.displayName : option.name);

    switch (option.type) {
      case ZoneOption::Integer:
        (new NumberEdit(
             line, rect_t{0, 0, 96, 0}, option.min.signedValue,
             option.max.signedValue,
             [=]() -> int {
               return widget->getOptionValue(optIdx)->signedValue;
             },
             [=](int32_t newValue) {
               widget->getOptionValue(optIdx)->signedValue = newValue;
               SET_DIRTY();
             }))
            ->setDefault(option.deflt.signedValue);
        break;

      case ZoneOption::Source:
        new SourceChoice(
            line, rect_t{}, 0, MIXSRC_LAST_TELEM,
            [=]() -> int16_t {
              return (int16_t)widget->getOptionValue(optIdx)->unsignedValue;
            },
            [=](int16_t newValue) {
              widget->getOptionValue(optIdx)->unsignedValue =
                  (uint32_t)newValue;
              SET_DIRTY();
            });
        break;

      case ZoneOption::Bool:
        new ToggleSwitch(
            line, rect_t{},
            [=]() -> uint8_t {
              return (uint8_t)widget->getOptionValue(optIdx)->boolValue;
            },
            [=](int8_t newValue) {
              widget->getOptionValue(optIdx)->boolValue = newValue;
              SET_DIRTY();
            });
        break;

      case ZoneOption::String:
        new ModelTextEdit(line, rect_t{0, 0, 96, 0},
                          widget->getOptionValue(optIdx)->stringValue,
                          sizeof(widget->getOptionValue(optIdx)->stringValue));
        break;

      case ZoneOption::File:
        break;

      case ZoneOption::TextSize:
        new Choice(
            line, rect_t{}, STR_FONT_SIZES, 0, FONTS_COUNT - 1,
            [=]() -> int {  // getValue
              return (int)widget->getOptionValue(optIdx)->unsignedValue;
            },
            [=](int newValue) {  // setValue
              widget->getOptionValue(optIdx)->unsignedValue =
                  (uint32_t)newValue;
              SET_DIRTY();
            });
        break;

      case ZoneOption::Align:
        new Choice(
            line, rect_t{}, STR_ALIGN_OPTS, 0, ALIGN_COUNT - 1,
            [=]() -> int {  // getValue
              return (int)widget->getOptionValue(optIdx)->unsignedValue;
            },
            [=](int newValue) {  // setValue
              widget->getOptionValue(optIdx)->unsignedValue =
                  (uint32_t)newValue;
              SET_DIRTY();
            });
        break;

      case ZoneOption::Timer:  // Unsigned
      {
        auto tmChoice = new Choice(
            line, rect_t{}, 0, TIMERS - 1,
            [=]() -> int {  // getValue
              return (int)widget->getOptionValue(optIdx)->unsignedValue;
            },
            [=](int newValue) {  // setValue
              widget->getOptionValue(optIdx)->unsignedValue =
                  (uint32_t)newValue;
              SET_DIRTY();
            });

        tmChoice->setTextHandler([](int value) {
          return std::string(STR_TIMER) + std::to_string(value + 1);
        });
      } break;

      case ZoneOption::Switch:
        new SwitchChoice(
            line, rect_t{},
            option.min.signedValue,  // min
            option.max.signedValue,  // max
            [=]() -> int16_t {       // getValue
              return widget->getOptionValue(optIdx)->signedValue;
            },
            [=](int16_t newValue) {  // setValue
              widget->getOptionValue(optIdx)->signedValue = newValue;
              SET_DIRTY();
            });
        break;

      case ZoneOption::Color:
        new ColorPicker(
            line, rect_t{},
            [=]() -> int {  // getValue
              return (int)widget->getOptionValue(optIdx)->unsignedValue;
            },
            [=](int newValue) {  // setValue
              widget->getOptionValue(optIdx)->unsignedValue =
                  (uint32_t)newValue;
              SET_DIRTY();
            });
        break;
    }

    optIdx++;
    optPtr++;
  }
}

void WidgetSettings::onCancel()
{
  widget->update();
  deleteLater();
}