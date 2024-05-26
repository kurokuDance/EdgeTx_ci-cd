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

#include "view_main.h"

#include "menu_model.h"
#include "menu_radio.h"
#include "menu_screen.h"
#include "model_select.h"
#include "opentx.h"
#include "theme.h"
#include "topbar_impl.h"
#include "view_channels.h"
#include "view_main_menu.h"

static void tile_view_deleted_cb(lv_event_t* e)
{
  lv_obj_t* target = lv_event_get_target(e);
  lv_obj_t* obj = lv_event_get_current_target(e);

  // LV_EVENT_CHILD_DELETED is bubbled to all parents, so
  // we'd better make sure this is one of our own.
  if (obj == target) {
    TRACE("CHILD_DELETED tile[%d]", lv_event_get_user_data(e));
    lv_obj_del(obj);
  }
}

static void saveViewId(unsigned view)
{
  if (view != g_model.view) {
    TRACE("save view #%d", view);
    g_model.view = view;
    storageDirty(EE_MODEL);
  }
}

static void tile_view_scroll(lv_event_t* e)
{
  // (void)e;
  auto viewMain = ViewMain::instance();
  if (viewMain) {
    if (lv_event_get_code(e) == LV_EVENT_SCROLL_END) {
      auto view = viewMain->getCurrentMainView();
      saveViewId(view);
    } else {
      viewMain->updateTopbarVisibility();
    }
  }
}

ViewMain* ViewMain::_instance = nullptr;

ViewMain::ViewMain() :
    NavWindow(MainWindow::instance(), MainWindow::instance()->getRect())
{
  Layer::push(this);

  tile_view = lv_tileview_create(lvobj);
  lv_obj_set_pos(tile_view, rect.x, rect.y);
  lv_obj_set_size(tile_view, rect.w, rect.h);
  lv_obj_set_scrollbar_mode(tile_view, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(tile_view, LV_OBJ_FLAG_SCROLL_ELASTIC);

  lv_obj_add_flag(tile_view, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_user_data(tile_view, this);
  lv_obj_add_event_cb(tile_view, tile_view_scroll, LV_EVENT_SCROLL, nullptr);
  lv_obj_add_event_cb(tile_view, tile_view_scroll, LV_EVENT_SCROLL_END,
                      nullptr);
  lv_obj_add_event_cb(lvobj, ViewMain::long_pressed, LV_EVENT_LONG_PRESSED,
                      nullptr);

  // create last to be on top
  topbar = TopbarFactory::create(this);
}

ViewMain::~ViewMain() { _instance = nullptr; }

void ViewMain::deleteLater(bool detach, bool trash)
{
  Layer::pop(this);
  Window::deleteLater(detach, trash);
}

void ViewMain::addMainView(Window* view, uint32_t viewId)
{
  TRACE("addMainView(0x%p, %d)", view, viewId);

  auto tile =
      lv_tileview_add_tile(tile_view, viewId, 0, LV_DIR_LEFT | LV_DIR_RIGHT);

  auto view_obj = view->getLvObj();
  lv_obj_set_parent(view_obj, tile);

  auto user_data = (void*)(intptr_t)viewId;
  lv_obj_add_event_cb(tile, tile_view_deleted_cb, LV_EVENT_CHILD_DELETED,
                      user_data);
}

void ViewMain::setTopbarVisible(float visible) { topbar->setVisible(visible); }

unsigned ViewMain::getMainViewsCount() const
{
  return lv_obj_get_child_cnt(tile_view);
}

rect_t ViewMain::getMainZone(rect_t zone, bool hasTopbar) const
{
  if (isVisible) {
    auto visibleHeight = topbar->getVisibleHeight(hasTopbar ? 1.0 : 0.0);
    zone.y += visibleHeight;
    zone.h -= visibleHeight;
    return zone;
  } else {
    return {0, 0, LCD_W, LCD_H};
  }
}

unsigned ViewMain::getCurrentMainView() const
{
  return lv_obj_get_scroll_x(tile_view) / width();
}

void ViewMain::setCurrentMainView(unsigned viewId)
{
  lv_obj_set_tile_id(tile_view, viewId, 0, LV_ANIM_OFF);
}

void setRequestedMainView(uint8_t view) { g_model.view = view; }

void ViewMain::nextMainView()
{
  auto view = getCurrentMainView();
  if (++view >= getMainViewsCount()) view = 0;

  setCurrentMainView(view);
  saveViewId(view);
}

void ViewMain::previousMainView()
{
  auto view = getCurrentMainView();
  if (view > 0)
    view--;
  else
    view = getMainViewsCount() - 1;

  setCurrentMainView(view);
  saveViewId(view);
}

TopBar* ViewMain::getTopbar() { return topbar; }

static bool hasTopbar(unsigned view)
{
  if (view < sizeof(g_model.screenData)) {
    const auto& layoutData = g_model.screenData[view].layoutData;
    return layoutData.options[LAYOUT_OPTION_TOPBAR].value.boolValue;
  }

  return false;
}

void ViewMain::enableTopbar()
{
  if (topbar) topbar->show();
}

void ViewMain::disableTopbar()
{
  if (topbar) topbar->hide();
}

void ViewMain::updateTopbarVisibility()
{
  if (!tile_view) return;

  coord_t scrollPos = lv_obj_get_scroll_x(tile_view);
  coord_t pageWidth = width();
  if (!pageWidth) return;

  int leftScroll = scrollPos % width();
  if (leftScroll == 0) {
    int view = scrollPos / pageWidth;
    setTopbarVisible(hasTopbar(view));
    if (customScreens[view]) customScreens[view]->adjustLayout();
  } else {
    int leftIdx = scrollPos / pageWidth;
    bool leftTopbar = hasTopbar(leftIdx);
    bool rightTopbar = hasTopbar(leftIdx + 1);

    if (leftTopbar != rightTopbar) {
      float ratio = (float)leftScroll / (float)pageWidth;

      if (leftTopbar) {
        // scrolling from a screen with Topbar
        ratio = 1.0 - ratio;
      } else {
        // scrolling to a screen with Topbar
        // -> ratio is ok
      }

      setTopbarVisible(ratio);
      customScreens[leftIdx]->adjustLayout();
      customScreens[leftIdx + 1]->adjustLayout();
    }
  }
}

#if defined(HARDWARE_KEYS)
void ViewMain::onPressSYS()
{
  if (viewMainMenu) viewMainMenu->onCancel();
  new RadioMenu();
}
void ViewMain::onLongPressSYS()
{
  if (viewMainMenu) viewMainMenu->onCancel();
  // Radio setup
  (new RadioMenu())->setCurrentTab(2);
}
void ViewMain::onPressMDL()
{
  if (viewMainMenu) viewMainMenu->onCancel();
  new ModelMenu();
}
void ViewMain::onLongPressMDL()
{
  if (viewMainMenu) viewMainMenu->onCancel();
  new ModelLabelsWindow();
}
void ViewMain::onPressTELE()
{
  if (viewMainMenu) viewMainMenu->onCancel();
  new ScreenMenu();
}
void ViewMain::onLongPressTELE()
{
  if (viewMainMenu) viewMainMenu->onCancel();
  new ChannelsViewMenu();
}
void ViewMain::onPressPGUP()
{
  if (!widget_select) {
    if (viewMainMenu) viewMainMenu->onCancel();
    previousMainView();
  }
}
void ViewMain::onPressPGDN()
{
  if (!widget_select) {
    if (viewMainMenu) viewMainMenu->onCancel();
    nextMainView();
  }
}
#endif

void ViewMain::onClicked() { openMenu(); }

void ViewMain::onCancel()
{
  if (widget_select) {
    enableWidgetSelect(false);
  }
}

void ViewMain::refreshWidgetSelectTimer()
{
  if (!widget_select_timer) {
    widget_select_timer = lv_timer_create(ViewMain::ws_timer, 10 * 1000, this);
  } else {
    lv_timer_reset(widget_select_timer);
  }
}

bool ViewMain::enableWidgetSelect(bool enable)
{
  TRACE("enableWidgetSelect(%d)", enable);
  // TODO: start timer
  if (widget_select == enable) return false;
  widget_select = enable;

  lv_obj_t* tile = lv_tileview_get_tile_act(tile_view);
  if (!tile) return true;

  auto cont_obj = lv_obj_get_child(tile, 0);
  if (!cont_obj) return true;

  auto cont = (WidgetsContainer*)lv_obj_get_user_data(cont_obj);

  for (uint32_t i = 0; i < cont->getZonesCount(); i++) {
    Widget* widget = cont->getWidget(i);
    if (widget)
      widget->enableFocus(enable);
  }

  if (enable) {
    lv_obj_clear_flag(tile_view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tile_view, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(tile_view, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
  } else {
    lv_obj_add_flag(tile_view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tile_view, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_flag(tile_view, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    if (widget_select_timer) {
      lv_timer_del(widget_select_timer);
      widget_select_timer = nullptr;
    }
  }

  return true;
}

void ViewMain::openMenu()
{
  viewMainMenu = new ViewMainMenu(this, [=]() { viewMainMenu = nullptr; });
}

void ViewMain::ws_timer(lv_timer_t* t)
{
  ViewMain* view = (ViewMain*)t->user_data;
  if (!view) return;
  view->enableWidgetSelect(false);
}

void ViewMain::long_pressed(lv_event_t* e)
{
  auto obj = lv_event_get_target(e);
  auto view = (ViewMain*)lv_obj_get_user_data(obj);
  if (!view) return;

  if (view->enableWidgetSelect(true)) {
    // kill subsequent CLICKED event
    lv_obj_clear_state(obj, LV_STATE_PRESSED);
    lv_indev_wait_release(lv_indev_get_act());
  }
}

void ViewMain::show(bool visible)
{
  isVisible = visible;
  coord_t scrollPos = lv_obj_get_scroll_x(tile_view);
  coord_t pageWidth = width();
  int view = scrollPos / pageWidth;
  setTopbarVisible(visible && hasTopbar(view));
  if (customScreens[view]) {
    customScreens[view]->show(visible);
    customScreens[view]->showWidgets(visible);
  }
}

void ViewMain::runBackground()
{
  topbar->runBackground();
  for (int i = 0; i < MAX_CUSTOM_SCREENS; i += 1) {
    if (customScreens[i])
      customScreens[i]->runBackground();
  }
}
