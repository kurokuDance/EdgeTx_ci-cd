/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   libopenui - https://github.com/opentx/libopenui
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

#include "button.h"

#include "theme.h"
#include "themes/etx_lv_theme.h"

static void button_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj)
{
  etx_btn_style(obj, LV_PART_MAIN);
}

// Must not be static - inherited by menu_button_class
const lv_obj_class_t button_class = {
    .base_class = &lv_btn_class,
    .constructor_cb = button_constructor,
    .destructor_cb = nullptr,
    .user_data = nullptr,
    .event_cb = nullptr,
    .width_def = LV_SIZE_CONTENT,
    .height_def = 32,
    .editable = LV_OBJ_CLASS_EDITABLE_INHERIT,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size = sizeof(lv_btn_t),
};

static lv_obj_t* button_create(lv_obj_t* parent)
{
  return etx_create(&button_class, parent);
}

Button::Button(Window* parent, const rect_t& rect,
               std::function<uint8_t(void)> pressHandler) :
    ButtonBase(parent, rect, pressHandler, button_create)
{
}

ButtonBase::ButtonBase(Window* parent, const rect_t& rect,
                       std::function<uint8_t(void)> pressHandler,
                       LvglCreate objConstruct) :
    FormField(parent, rect, 0, objConstruct ? objConstruct : lv_btn_create),
    pressHandler(std::move(pressHandler))
{
  lv_obj_add_event_cb(lvobj, ButtonBase::long_pressed, LV_EVENT_LONG_PRESSED,
                      nullptr);
}

void ButtonBase::check(bool checked)
{
  if (!_deleted) {
    if (checked != this->checked()) {
      if (checked)
        lv_obj_add_state(lvobj, LV_STATE_CHECKED);
      else
        lv_obj_clear_state(lvobj, LV_STATE_CHECKED);
    }
  }
}

bool ButtonBase::checked() const
{
  return lv_obj_get_state(lvobj) & LV_STATE_CHECKED;
}

void ButtonBase::onPress() { check(pressHandler && pressHandler()); }

void ButtonBase::onLongPress()
{
  if (longPressHandler) {
    check(longPressHandler());
    lv_obj_clear_state(lvobj, LV_STATE_PRESSED);
    lv_indev_wait_release(lv_indev_get_act());
  }
}

void ButtonBase::onClicked() { onPress(); }

void ButtonBase::checkEvents()
{
  Window::checkEvents();
  if (checkHandler) checkHandler();
}

void ButtonBase::long_pressed(lv_event_t* e)
{
  auto obj = lv_event_get_target(e);
  auto btn = (ButtonBase*)lv_obj_get_user_data(obj);
  if (obj) btn->onLongPress();
}

TextButton::TextButton(Window* parent, const rect_t& rect, std::string text,
                       std::function<uint8_t(void)> pressHandler) :
    ButtonBase(parent, rect, pressHandler, button_create), text(std::move(text))
{
  label = lv_label_create(lvobj);
  lv_label_set_text(label, this->text.c_str());
  lv_obj_center(label);
}

IconButton::IconButton(Window* parent, EdgeTxIcon icon,
                       std::function<uint8_t(void)> pressHandler) :
    ButtonBase(parent, {0, 0, 32, 32}, pressHandler, button_create)
{
  padAll(PAD_ZERO);
  iconImage = new StaticIcon(this, 0, 0, icon, COLOR_THEME_SECONDARY1);
  iconImage->center(28, 28);
}

void IconButton::setIcon(EdgeTxIcon icon) { iconImage->setIcon(icon); }
