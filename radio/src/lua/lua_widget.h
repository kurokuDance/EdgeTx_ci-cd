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

#include "window.h"
#include "widget.h"
#include "lua_api.h"

#include "opentx_types.h"

#define LUA_TAP_TIME 250 // 250 ms

class LuaEventHandler
{
#if defined(HARDWARE_TOUCH)
  // "tap" handling
  static uint32_t downTime;
  static uint32_t tapTime;
  static uint32_t tapCount;
  // "swipe" / "slide" handling
  static tmr10ms_t swipeTimeOut;
  static bool _sliding;
  static coord_t _startX;
  static coord_t _startY;
#endif
  static void event_cb(lv_event_t* e);

protected:
  void onClicked();
  void onCancel();
  void onEvent(event_t event);

public:
  LuaEventHandler() = default;
  void setupHandler(Window* w);
  void removeHandler(Window* w);
};

class LuaWidget : public Widget, public LuaEventHandler
{
  friend class LuaWidgetFactory;

  int luaWidgetDataRef;
  int zoneRectDataRef;
  char* errorMessage;
  bool refreshed = false;

  // Window interface
  void onClicked() override;
  void onCancel() override;
  void checkEvents() override;

  // Widget interface
  void onFullscreen(bool enable) override;
    
  void setErrorMessage(const char* funcName);

  // Update 'zone' data
  void updateZoneRect(rect_t rect) override;
  bool updateTable(const char* idx, int val);

 public:
  LuaWidget(const WidgetFactory* factory, Window* parent, const rect_t& rect,
            WidgetPersistentData* persistentData, int luaWidgetDataRef, int zoneRectDataRef);
  ~LuaWidget() override;

#if defined(DEBUG_WINDOWS)
  std::string getName() const override { return "LuaWidget"; }
#endif

  // Widget interface
  const char* getErrorMessage() const override;
  void update() override;
  void background() override;

 protected:
  // Calls LUA widget 'refresh' method
  void refresh(BitmapBuffer* dc);

  // Window interface
  void onEvent(event_t event) override;

  static void redraw_cb(lv_event_t *e);
};
