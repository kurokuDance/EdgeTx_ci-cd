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

#include "topbar_impl.h"
#include "opentx.h"
#include "theme.h"

constexpr uint32_t TOPBAR_REFRESH = 1000 / 10; // 10 Hz

TopBar::TopBar(Window * parent) :
  TopBarBase(parent, {0, 0, LCD_W, MENU_HEADER_HEIGHT}, &g_model.topbarData)
{
  etx_solid_bg(lvobj, COLOR_THEME_SECONDARY1_INDEX);

  headerIcon = new HeaderIcon(this, ICON_EDGETX);
}

unsigned int TopBar::getZonesCount() const
{
  return MAX_TOPBAR_ZONES;
}

rect_t TopBar::getZone(unsigned int index) const
{
#if LCD_H > LCD_W
  if (index == MAX_TOPBAR_ZONES - 1) {
    coord_t size = LCD_W - 48 - (MAX_TOPBAR_ZONES - 1) * (TOPBAR_ZONE_WIDTH + TOPBAR_ZONE_HMARGIN);
    return {LCD_W - size, TOPBAR_ZONE_VMARGIN, size, TOPBAR_ZONE_HEIGHT};
  }
#endif

  return {
    coord_t(48 + (TOPBAR_ZONE_WIDTH + TOPBAR_ZONE_HMARGIN) * index),
    TOPBAR_ZONE_VMARGIN,
    TOPBAR_ZONE_WIDTH,
    TOPBAR_ZONE_HEIGHT
  };
}

void TopBar::setVisible(float visible) // 0.0 -> 1.0
{
  if (visible == 0.0) {
    setTop(-(int)MENU_HEADER_HEIGHT);
  }
  else if (visible == 1.0) {
    setTop(0);
  }
  else if (visible > 0.0 && visible < 1.0){
    float top = - (float)MENU_HEADER_HEIGHT * (1.0 - visible);
    setTop((coord_t)top);
  }
}

coord_t TopBar::getVisibleHeight(float visible) const // 0.0 -> 1.0
{
  if (visible == 0.0) {
    return 0;
  }
  else if (visible == 1.0) {
    return MENU_HEADER_HEIGHT;
  }

  float h = (float)MENU_HEADER_HEIGHT * visible;
  return (coord_t)h;
}

void TopBar::checkEvents()
{
  uint32_t now = RTOS_GET_MS();
  if (now - lastRefresh >= TOPBAR_REFRESH) {
    lastRefresh = now;
    TopBarBase::checkEvents();
  }
}

void TopBar::removeWidget(unsigned int index)
{
  bool mark = false;

  // If user manually removes 'system' widgets, mark name so widget does not get reloaded on restart
  if ((index == MAX_TOPBAR_ZONES - 1) && (strncmp(persistentData->zones[index].widgetName, "Date Time", WIDGET_NAME_LEN) == 0))
    mark = true;
  if ((index == MAX_TOPBAR_ZONES - 2) && (strncmp(persistentData->zones[index].widgetName, "Radio Info", WIDGET_NAME_LEN) == 0))
    mark = true;
#if defined(INTERNAL_GPS)
  if ((index == MAX_TOPBAR_ZONES - 3) && (strncmp(persistentData->zones[index].widgetName, "Internal GPS", WIDGET_NAME_LEN) == 0))
    mark = true;
#endif
  
  WidgetsContainerImpl::removeWidget(index);

  // If user manually removes 'system' widgets, mark name so widget does not get reloaded on restart
  if (mark)
    strcpy(persistentData->zones[index].widgetName, "--");
}
