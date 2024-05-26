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

#include "color_picker.h"

#include "color_editor.h"
#include "color_list.h"
#include "themes/etx_lv_theme.h"

// based on LVGL default switch size
constexpr lv_coord_t COLOR_PAD_WIDTH = (4 * LV_DPI_DEF) / 10;
constexpr lv_coord_t COLOR_PAD_HEIGHT = 32;

#if LCD_W > LCD_H
// Landscape
static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                     LV_GRID_TEMPLATE_LAST};
static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

#define COLOR_EDIT_WIDTH LCD_W * 0.8

#else
// Portrait
static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT,
                                     LV_GRID_TEMPLATE_LAST};

#define COLOR_EDIT_WIDTH LCD_W * 0.7

#endif

class ColorEditorPopup : public BaseDialog
{
  ColorSwatch* colorPad = nullptr;
  StaticText* hexStr = nullptr;
  uint32_t m_color;

  void updateColor(uint32_t c)
  {
    m_color = c;
    uint8_t r = GET_RED(c), g = GET_GREEN(c), b = GET_BLUE(c);

    colorPad->setColor(r, g, b);

    char s[10];
    sprintf(s, "%02X%02X%02X", r, g, b);
    hexStr->setText(s);
  }

 public:
  ColorEditorPopup(Window* parent, uint32_t color,
                   std::function<void(uint32_t)> _setValue) :
      BaseDialog(parent, STR_COLOR_PICKER, false, COLOR_EDIT_WIDTH,
                 LV_SIZE_CONTENT)
  {
    FlexGridLayout grid(col_dsc, row_dsc);
    auto line = form->newLine(grid);

    rect_t r{0, 0, 7 * LV_DPI_DEF / 5, 7 * LV_DPI_DEF / 5};
    auto cedit =
        new ColorEditor(line, r, color, [=](uint32_t c) { updateColor(c); });
    lv_obj_set_style_grid_cell_x_align(cedit->getLvObj(), LV_GRID_ALIGN_CENTER,
                                       0);

    auto vbox = new Window(line, rect_t{});
    lv_obj_set_style_grid_cell_x_align(vbox->getLvObj(), LV_GRID_ALIGN_CENTER,
                                       0);
    vbox->setFlexLayout(LV_FLEX_FLOW_COLUMN, PAD_MEDIUM, r.w, r.h);

    auto hbox = new Window(vbox, rect_t{});
    hbox->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_MEDIUM);
    auto hbox_obj = hbox->getLvObj();
    lv_obj_set_flex_align(hbox_obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_SPACE_AROUND);

    colorPad = new ColorSwatch(hbox, {0, 0, COLOR_PAD_WIDTH, COLOR_PAD_HEIGHT},
                               COLOR_THEME_PRIMARY1);

    hexStr = new StaticText(hbox, {0, 0, 100, 0}, "", COLOR_THEME_PRIMARY1 | FONT(L));

    updateColor(color);

    hbox = new Window(vbox, rect_t{});
    hbox->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_MEDIUM);
    lv_obj_set_flex_align(hbox->getLvObj(), LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);

    auto rgbBtn = new TextButton(hbox, rect_t{}, "RGB");
    auto hsvBtn = new TextButton(hbox, rect_t{}, "HSV");
    auto thmBtn = new TextButton(hbox, rect_t{}, "SYS");

    rgbBtn->setPressHandler([=]() {
      cedit->setColorEditorType(RGB_COLOR_EDITOR);
      hsvBtn->check(false);
      thmBtn->check(false);
      return 1;
    });
    rgbBtn->padAll(PAD_MEDIUM);

    hsvBtn->setPressHandler([=]() {
      cedit->setColorEditorType(HSV_COLOR_EDITOR);
      rgbBtn->check(false);
      thmBtn->check(false);
      return 1;
    });
    hsvBtn->padAll(PAD_MEDIUM);

    thmBtn->setPressHandler([=]() {
      cedit->setColorEditorType(THM_COLOR_EDITOR);
      rgbBtn->check(false);
      hsvBtn->check(false);
      return 1;
    });
    thmBtn->padAll(PAD_MEDIUM);

    // color editor defaults to HSV
    hsvBtn->check(true);

    hbox = new Window(vbox, rect_t{});
    hbox->padTop(60);
    hbox->setFlexLayout(LV_FLEX_FLOW_ROW, 20);
    lv_obj_set_flex_align(hbox->getLvObj(), LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_flex_grow(hbox->getLvObj(), 1);

    new TextButton(hbox, rect_t{0, 0, 80, 0}, STR_CANCEL, [=]() -> int8_t {
      this->deleteLater();
      return 0;
    });

    new TextButton(hbox, rect_t{0, 0, 80, 0}, STR_SAVE, [=]() -> int8_t {
      if (_setValue) _setValue(m_color);
      this->deleteLater();
      return 0;
    });
  }
};

ColorPicker::ColorPicker(Window* parent, const rect_t& rect,
                         std::function<uint16_t()> getValue,
                         std::function<void(uint16_t)> setValue) :
    Button(parent, {0, 0, COLOR_PAD_WIDTH, COLOR_PAD_HEIGHT}),
    setValue(std::move(setValue))
{
  updateColor(getValue());
}

void ColorPicker::onClicked()
{
  new ColorEditorPopup(this, getColor(), [=](uint32_t c) { setColor(c); });
}

void ColorPicker::setColor(uint32_t c)
{
  setValue(c);
  updateColor(c);
}

void ColorPicker::updateColor(uint32_t c)
{
  color = c;

  auto lvcolor =
      lv_color_make(GET_RED(color), GET_GREEN(color), GET_BLUE(color));
  lv_obj_set_style_bg_color(lvobj, lvcolor, LV_PART_MAIN);
}
