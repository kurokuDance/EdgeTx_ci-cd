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

#include "button.h"
#include "opentx_types.h"
#include "tabsgroup.h"

class ListLineButton : public ButtonBase
{
 public:
  ListLineButton(Window* parent, uint8_t index);

  uint8_t getIndex() const { return index; }
  virtual void setIndex(uint8_t i) { index = i; }

  void checkEvents() override;

  virtual void refresh() = 0;

 protected:
  uint8_t index;

  virtual bool isActive() const = 0;
};

class InputMixButtonBase : public ListLineButton
{
 public:
  InputMixButtonBase(Window* parent, uint8_t index);
  ~InputMixButtonBase();

  void setWeight(gvar_t value, gvar_t min, gvar_t max);
  void setSource(mixsrc_t idx);
  void setFlightModes(uint16_t modes);

  static constexpr coord_t BTN_H = 29;

  virtual void updatePos(coord_t x, coord_t y) = 0;
  virtual void swapLvglGroup(InputMixButtonBase* line2) = 0;

 protected:

  lv_obj_t* fm_canvas = nullptr;
  void* fm_buffer = nullptr;
  uint16_t fm_modes = 0;

  lv_obj_t* weight = nullptr;
  lv_obj_t* source = nullptr;
  lv_obj_t* opts = nullptr;
};

class InputMixGroupBase : public Window
{
 public:
  InputMixGroupBase(Window* parent, mixsrc_t idx);

  mixsrc_t getMixSrc() { return idx; }
  size_t getLineCount() { return lines.size(); }

  virtual void adjustHeight();
  void addLine(InputMixButtonBase* line);
  bool removeLine(InputMixButtonBase* line);

  void refresh();

 protected:
  static constexpr coord_t LN_X = 73;

  mixsrc_t idx;
  lv_obj_t* label;
  std::list<InputMixButtonBase*> lines;
};

class InputMixPageBase : public PageTab
{
 public:
  InputMixPageBase(const char* title, EdgeTxIcon icon) : PageTab(title, icon) {}

 protected:
  std::list<InputMixButtonBase*> lines;
  InputMixButtonBase* _copySrc = nullptr;
  Window* form = nullptr;
  uint8_t _copyMode = 0;
  std::list<InputMixGroupBase*> groups;

  virtual void addLineButton(uint8_t index) = 0;
  void addLineButton(mixsrc_t src, uint8_t index);

  InputMixButtonBase* getLineByIndex(uint8_t index);
  InputMixGroupBase* getGroupBySrc(mixsrc_t src);
  virtual InputMixGroupBase* getGroupByIndex(uint8_t index) = 0;

  virtual InputMixButtonBase* createLineButton(InputMixGroupBase* group, uint8_t index) = 0;
  virtual InputMixGroupBase* createGroup(Window* form, mixsrc_t src) = 0;

  void removeLine(InputMixButtonBase* l);
  void removeGroup(InputMixGroupBase* g);
};
