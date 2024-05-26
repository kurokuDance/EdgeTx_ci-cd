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

/*********************
 *      INCLUDES
 *********************/
#include "etx_lv_theme.h"

#include "../colors.h"
#include "fonts.h"

extern lv_color_t makeLvColor(uint32_t colorFlags);

static lv_theme_t theme;

/**********************
 *   Constant Styles
 **********************/

// Opacity
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::bg_opacity_transparent,
                           LV_STYLE_BG_OPA, LV_OPA_TRANSP);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::bg_opacity_20, LV_STYLE_BG_OPA,
                           LV_OPA_20);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::bg_opacity_50, LV_STYLE_BG_OPA,
                           LV_OPA_50);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::bg_opacity_75, LV_STYLE_BG_OPA, 187);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::bg_opacity_cover, LV_STYLE_BG_OPA,
                           LV_OPA_COVER);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::fg_opacity_transparent, LV_STYLE_OPA,
                           LV_OPA_TRANSP);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::fg_opacity_cover, LV_STYLE_OPA,
                           LV_OPA_COVER);

// Corner rounding (button, edit box, etc)
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::rounded, LV_STYLE_RADIUS, 6);

// Toggle switch and slider knob rounding
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::circle, LV_STYLE_RADIUS,
                           LV_RADIUS_CIRCLE);

// Text align
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::text_align_left, LV_STYLE_TEXT_ALIGN,
                           LV_TEXT_ALIGN_LEFT);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::text_align_right, LV_STYLE_TEXT_ALIGN,
                           LV_TEXT_ALIGN_RIGHT);
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::text_align_center, LV_STYLE_TEXT_ALIGN,
                           LV_TEXT_ALIGN_CENTER);

// Padding
LV_STYLE_CONST_SINGLE_INIT(EdgeTxStyles::pad_left_2, LV_STYLE_PAD_LEFT, 2);

// Scrollbar
const lv_style_const_prop_t scrollbar_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_50),
    LV_STYLE_CONST_PAD_TOP(3),  LV_STYLE_CONST_PAD_BOTTOM(3),
    LV_STYLE_CONST_PAD_LEFT(3), LV_STYLE_CONST_PAD_RIGHT(3),
    LV_STYLE_CONST_WIDTH(4),    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::scrollbar, scrollbar_props);

// Padding
const lv_style_const_prop_t pad_zero_props[] = {
    LV_STYLE_CONST_PAD_TOP(0),  LV_STYLE_CONST_PAD_BOTTOM(0),
    LV_STYLE_CONST_PAD_LEFT(0), LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_PAD_ROW(0),  LV_STYLE_CONST_PAD_COLUMN(0),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_zero, pad_zero_props);

const lv_style_const_prop_t pad_tiny_props[] = {
    LV_STYLE_CONST_PAD_TOP(2),  LV_STYLE_CONST_PAD_BOTTOM(2),
    LV_STYLE_CONST_PAD_LEFT(2), LV_STYLE_CONST_PAD_RIGHT(2),
    LV_STYLE_CONST_PAD_ROW(2),  LV_STYLE_CONST_PAD_COLUMN(2),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_tiny, pad_tiny_props);

const lv_style_const_prop_t pad_small_props[] = {
    LV_STYLE_CONST_PAD_TOP(4),  LV_STYLE_CONST_PAD_BOTTOM(4),
    LV_STYLE_CONST_PAD_LEFT(4), LV_STYLE_CONST_PAD_RIGHT(4),
    LV_STYLE_CONST_PAD_ROW(4),  LV_STYLE_CONST_PAD_COLUMN(4),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_small, pad_small_props);

const lv_style_const_prop_t pad_medium_props[] = {
    LV_STYLE_CONST_PAD_TOP(6),  LV_STYLE_CONST_PAD_BOTTOM(6),
    LV_STYLE_CONST_PAD_LEFT(6), LV_STYLE_CONST_PAD_RIGHT(6),
    LV_STYLE_CONST_PAD_ROW(4),  LV_STYLE_CONST_PAD_COLUMN(4),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_medium, pad_medium_props);

const lv_style_const_prop_t pad_large_props[] = {
    LV_STYLE_CONST_PAD_TOP(8),  LV_STYLE_CONST_PAD_BOTTOM(8),
    LV_STYLE_CONST_PAD_LEFT(8), LV_STYLE_CONST_PAD_RIGHT(8),
    LV_STYLE_CONST_PAD_ROW(4),  LV_STYLE_CONST_PAD_COLUMN(4),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_large, pad_large_props);

const lv_style_const_prop_t pad_button_props[] = {
    LV_STYLE_CONST_PAD_TOP(2),  LV_STYLE_CONST_PAD_BOTTOM(2),
    LV_STYLE_CONST_PAD_LEFT(6), LV_STYLE_CONST_PAD_RIGHT(6),
    LV_STYLE_CONST_PAD_ROW(2),  LV_STYLE_CONST_PAD_COLUMN(2),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_button, pad_button_props);

const lv_style_const_prop_t pad_textarea_props[] = {
    LV_STYLE_CONST_PAD_TOP(4),  LV_STYLE_CONST_PAD_BOTTOM(3),
    LV_STYLE_CONST_PAD_LEFT(4), LV_STYLE_CONST_PAD_RIGHT(4),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pad_textarea, pad_textarea_props);

// Border
const lv_style_const_prop_t border_props[] = {
    LV_STYLE_CONST_BORDER_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_BORDER_WIDTH(2),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::border, border_props);

// Border
const lv_style_const_prop_t border_transparent_props[] = {
    LV_STYLE_CONST_BORDER_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_BORDER_WIDTH(2),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::border_transparent,
                          border_transparent_props);

const lv_style_const_prop_t border_thin_props[] = {
    LV_STYLE_CONST_BORDER_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_BORDER_WIDTH(1),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::border_thin, border_thin_props);

static const lv_style_const_prop_t outline_props[] = {
    LV_STYLE_CONST_OUTLINE_WIDTH(2),
    LV_STYLE_CONST_OUTLINE_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_OUTLINE_PAD(1),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::outline, outline_props);

static const lv_style_const_prop_t outline_thick_props[] = {
    LV_STYLE_CONST_OUTLINE_WIDTH(3),
    LV_STYLE_CONST_OUTLINE_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_OUTLINE_PAD(1),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::outline_thick, outline_thick_props);

// States (pressed, disabled, etc)
static lv_color_t dark_color_filter_cb(const lv_color_filter_dsc_t* f,
                                       lv_color_t c, lv_opa_t opa)
{
  LV_UNUSED(f);
  return lv_color_darken(c, opa);
}

const lv_color_filter_dsc_t dark_filter = {.filter_cb = dark_color_filter_cb,
                                           .user_data = 0};

const lv_style_const_prop_t pressed_props[] = {
    LV_STYLE_CONST_COLOR_FILTER_DSC(&dark_filter),
    LV_STYLE_CONST_COLOR_FILTER_OPA(35),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::pressed, pressed_props);

static lv_color_t grey_filter_cb(const lv_color_filter_dsc_t* f,
                                 lv_color_t color, lv_opa_t opa)
{
  LV_UNUSED(f);
  return lv_color_mix(lv_palette_lighten(LV_PALETTE_GREY, 2), color, opa);
}

const lv_color_filter_dsc_t grey_filter = {.filter_cb = grey_filter_cb,
                                           .user_data = 0};

const lv_style_const_prop_t disabled_props[] = {
    LV_STYLE_CONST_COLOR_FILTER_DSC(&grey_filter),
    LV_STYLE_CONST_COLOR_FILTER_OPA(LV_OPA_50),
    LV_STYLE_PROP_INV,
};
LV_STYLE_CONST_MULTI_INIT(EdgeTxStyles::disabled, disabled_props);

/**********************
 *   Variable Styles
 **********************/

EdgeTxStyles::EdgeTxStyles()
{
  for (int i = 0; i < TOTAL_COLOR_COUNT; i += 1) {
    lv_style_init(&bg_color[i]);
    lv_style_init(&txt_color[i]);
    lv_style_init(&img_color[i]);
    lv_style_set_img_recolor_opa(&img_color[i], LV_OPA_COVER);
  }
  lv_style_init(&border_color_dark);
  lv_style_init(&border_color_normal);
  lv_style_init(&border_color_focus);
  lv_style_init(&border_color_active);
  lv_style_init(&outline_color_light);
  lv_style_init(&outline_color_normal);
  lv_style_init(&outline_color_focus);
  lv_style_init(&arc_color);
  lv_style_init(&graph_border);
  lv_style_init(&graph_dashed);
  lv_style_init(&graph_line);
  lv_style_init(&graph_position_line);
  lv_style_init(&div_line);
  lv_style_init(&div_line_edit);
  lv_style_init(&div_line_black);
  lv_style_init(&div_line_white);

  lv_style_set_line_width(&graph_border, 1);
  lv_style_set_line_opa(&graph_border, LV_OPA_COVER);

  lv_style_set_line_width(&graph_dashed, 1);
  lv_style_set_line_opa(&graph_dashed, LV_OPA_COVER);
  lv_style_set_line_dash_width(&graph_dashed, 2);
  lv_style_set_line_dash_gap(&graph_dashed, 2);

  lv_style_set_line_width(&graph_line, 3);
  lv_style_set_line_opa(&graph_line, LV_OPA_COVER);
  lv_style_set_line_rounded(&graph_line, true);

  lv_style_set_line_width(&graph_position_line, 1);
  lv_style_set_line_opa(&graph_position_line, LV_OPA_COVER);

  lv_style_set_line_width(&div_line, 1);
  lv_style_set_line_opa(&div_line, LV_OPA_COVER);

  lv_style_set_line_width(&div_line_edit, 1);
  lv_style_set_line_opa(&div_line_edit, LV_OPA_COVER);

  lv_style_set_line_width(&div_line_black, 1);
  lv_style_set_line_opa(&div_line_black, LV_OPA_COVER);

  lv_style_set_line_width(&div_line_white, 1);
  lv_style_set_line_opa(&div_line_white, LV_OPA_COVER);

  lv_style_init(&bg_color_grey);
  lv_style_set_bg_color(&bg_color_grey, lv_palette_main(LV_PALETTE_GREY));
  lv_style_init(&border_color_black);
  lv_style_set_border_color(&border_color_black, lv_color_black());
  lv_style_init(&border_color_white);
  lv_style_set_border_color(&border_color_white, lv_color_white());

  // Fonts
  for (int i = FONT_STD_INDEX; i < FONTS_COUNT; i += 1) {
    lv_style_init(&font[i]);
  }

  applyColors();
}

void EdgeTxStyles::init()
{
  if (!initDone) {
    initDone = true;

    // Fonts
    for (int i = FONT_STD_INDEX; i < FONTS_COUNT; i += 1) {
      lv_style_set_text_font(&font[i], getFont(i << 8));
    }
  }

  applyColors();
}

void EdgeTxStyles::applyColors()
{
  // Always update colors in case theme changes

  for (int i = 0; i < TOTAL_COLOR_COUNT; i += 1) {
    lv_style_set_bg_color(&bg_color[i], makeLvColor(COLOR(i)));
    lv_style_set_text_color(&txt_color[i], makeLvColor(COLOR(i)));
    lv_style_set_img_recolor(&img_color[i], makeLvColor(COLOR(i)));
  }

  lv_style_set_line_color(&graph_border, makeLvColor(COLOR_THEME_SECONDARY2));
  lv_style_set_line_color(&graph_dashed,
                          makeLvColor(COLOR_THEME_SECONDARY2));
  lv_style_set_line_color(&graph_line, makeLvColor(COLOR_THEME_SECONDARY1));
  lv_style_set_line_color(&graph_position_line,
                          makeLvColor(COLOR_THEME_ACTIVE));
  lv_style_set_line_color(&div_line, makeLvColor(COLOR_THEME_SECONDARY1));
  lv_style_set_line_color(&div_line_edit, makeLvColor(COLOR_THEME_EDIT));
  lv_style_set_line_color(&div_line_black, makeLvColor(COLOR_THEME_PRIMARY1));
  lv_style_set_line_color(&div_line_white, makeLvColor(COLOR_THEME_PRIMARY2));

  lv_style_set_border_color(&border_color_dark,
                            makeLvColor(COLOR_THEME_SECONDARY1));
  lv_style_set_border_color(&border_color_normal,
                            makeLvColor(COLOR_THEME_SECONDARY2));
  lv_style_set_border_color(&border_color_focus,
                            makeLvColor(COLOR_THEME_FOCUS));
  lv_style_set_border_color(&border_color_active,
                            makeLvColor(COLOR_THEME_ACTIVE));

  lv_style_set_outline_color(&outline_color_light,
                             makeLvColor(COLOR_THEME_SECONDARY3));
  lv_style_set_outline_color(&outline_color_normal,
                             makeLvColor(COLOR_THEME_SECONDARY2));
  lv_style_set_outline_color(&outline_color_focus,
                             makeLvColor(COLOR_THEME_FOCUS));

  lv_style_set_arc_color(&arc_color, makeLvColor(COLOR_THEME_SECONDARY1));
}

static EdgeTxStyles mainStyles;
static EdgeTxStyles* previewStyles;
EdgeTxStyles* styles = &mainStyles;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_theme_t* etx_lv_theme_init(lv_disp_t* disp, lv_color_t color_primary,
                              lv_color_t color_secondary, const lv_font_t* font)
{
  theme.disp = disp;
  theme.color_primary = color_primary;
  theme.color_secondary = color_secondary;
  theme.font_small = font;
  theme.font_normal = font;
  theme.font_large = font;
  theme.flags = 0;

  styles->init();

  if (disp == NULL || lv_disp_get_theme(disp) == &theme)
    lv_obj_report_style_change(NULL);

  return (lv_theme_t*)&theme;
}

void usePreviewStyle()
{
  if (!previewStyles) previewStyles = new EdgeTxStyles();
  styles = previewStyles;
  styles->init();
}

void useMainStyle() { styles = &mainStyles; }

/**********************
 *   Custom object creation
 **********************/

// Object constructor helpers

void etx_solid_bg(lv_obj_t* obj, LcdColorIndex bg_color,
                  lv_style_selector_t selector)
{
  etx_bg_color(obj, bg_color, selector);
  etx_obj_add_style(obj, styles->bg_opacity_cover, selector);
}

void etx_font(lv_obj_t* obj, FontIndex fontIdx, lv_style_selector_t selector)
{
  // Remove old style first
  for (int i = 0; i < TOTAL_COLOR_COUNT; i += 1)
    lv_obj_remove_style(obj, &styles->font[i], selector);
  etx_obj_add_style(obj, styles->font[fontIdx], selector);
}

void etx_bg_color(lv_obj_t* obj, LcdColorIndex colorIdx,
                  lv_style_selector_t selector)
{
  // Remove old style first
  for (int i = 0; i < TOTAL_COLOR_COUNT; i += 1)
    lv_obj_remove_style(obj, &styles->bg_color[i], selector);
  etx_obj_add_style(obj, styles->bg_color[colorIdx], selector);
}

void etx_txt_color(lv_obj_t* obj, LcdColorIndex colorIdx,
                   lv_style_selector_t selector)
{
  // Remove old style first
  for (int i = 0; i < TOTAL_COLOR_COUNT; i += 1)
    lv_obj_remove_style(obj, &styles->txt_color[i], selector);
  etx_obj_add_style(obj, styles->txt_color[colorIdx], selector);
}

void etx_img_color(lv_obj_t* obj, LcdColorIndex colorIdx,
                   lv_style_selector_t selector)
{
  // Remove old style first
  for (int i = 0; i < TOTAL_COLOR_COUNT; i += 1)
    lv_obj_remove_style(obj, &styles->img_color[i], selector);
  etx_obj_add_style(obj, styles->img_color[colorIdx], selector);
}

void etx_std_ctrl_colors(lv_obj_t* obj, lv_style_selector_t selector)
{
  etx_solid_bg(obj, COLOR_THEME_PRIMARY2_INDEX, selector);
  etx_txt_color(obj, COLOR_THEME_SECONDARY1_INDEX, selector);

  etx_obj_add_style(obj, styles->border_color_focus,
                    selector | LV_STATE_FOCUSED);

  etx_bg_color(obj, COLOR_THEME_ACTIVE_INDEX, selector | LV_STATE_CHECKED);
  etx_txt_color(obj, COLOR_THEME_PRIMARY1_INDEX, selector | LV_STATE_CHECKED);

  etx_obj_add_style(obj, styles->outline_color_focus,
                    selector | LV_STATE_FOCUSED);
}

void etx_std_settings(lv_obj_t* obj, lv_style_selector_t selector)
{
  etx_obj_add_style(obj, styles->border, selector);
  etx_obj_add_style(obj, styles->border_color_normal, selector);
  etx_obj_add_style(obj, styles->rounded, selector);

  etx_obj_add_style(obj, styles->outline, selector | LV_STATE_FOCUSED);
  etx_obj_add_style(obj, styles->disabled, selector | LV_STATE_DISABLED);
  etx_obj_add_style(obj, styles->pressed, selector | LV_STATE_PRESSED);
}

void etx_btn_style(lv_obj_t* obj, lv_style_selector_t selector)
{
  etx_std_settings(obj, selector);
  etx_std_ctrl_colors(obj, selector);
  etx_obj_add_style(obj, styles->pad_button, selector);
}

void etx_padding(lv_obj_t* obj, PaddingSize padding,
                 lv_style_selector_t selector)
{
  lv_obj_remove_style(obj, (lv_style_t*)&styles->pad_tiny, selector);
  lv_obj_remove_style(obj, (lv_style_t*)&styles->pad_small, selector);
  lv_obj_remove_style(obj, (lv_style_t*)&styles->pad_medium, selector);
  lv_obj_remove_style(obj, (lv_style_t*)&styles->pad_large, selector);
  lv_obj_remove_style(obj, (lv_style_t*)&styles->pad_zero, selector);
  lv_obj_remove_style(obj, (lv_style_t*)&styles->pad_button, selector);
  switch (padding) {
    case PAD_TINY:
      etx_obj_add_style(obj, styles->pad_tiny, selector);
      break;
    case PAD_SMALL:
      etx_obj_add_style(obj, styles->pad_small, selector);
      break;
    case PAD_MEDIUM:
      etx_obj_add_style(obj, styles->pad_medium, selector);
      break;
    case PAD_LARGE:
      etx_obj_add_style(obj, styles->pad_large, selector);
      break;
    default:
      etx_obj_add_style(obj, styles->pad_zero, selector);
      break;
  };
}

void etx_std_style(lv_obj_t* obj, lv_style_selector_t selector,
                   PaddingSize padding)
{
  etx_std_settings(obj, selector);
  etx_std_ctrl_colors(obj, selector);
  etx_padding(obj, padding, selector);
}

void etx_scrollbar(lv_obj_t* obj)
{
  etx_obj_add_style(obj, styles->scrollbar, LV_PART_SCROLLBAR);
  etx_obj_add_style(obj, styles->bg_color_grey, LV_PART_SCROLLBAR);
  etx_obj_add_style(obj, styles->bg_opacity_cover,
                    LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);
}

void etx_textarea_style(lv_obj_t* obj)
{
  etx_std_settings(obj, LV_PART_MAIN);
  etx_std_ctrl_colors(obj, LV_PART_MAIN);
  etx_obj_add_style(obj, styles->pad_textarea, LV_PART_MAIN);

  etx_bg_color(obj, COLOR_THEME_EDIT_INDEX, LV_PART_MAIN | LV_STATE_EDITED);
  etx_txt_color(obj, COLOR_THEME_PRIMARY2_INDEX,
                LV_PART_MAIN | LV_STATE_EDITED);

  etx_obj_add_style(obj, styles->bg_opacity_50,
                    LV_PART_CURSOR | LV_STATE_EDITED);

  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
  lv_textarea_set_password_mode(obj, false);
  lv_textarea_set_one_line(obj, true);

  lv_textarea_t* ta = (lv_textarea_t*)obj;
  lv_obj_set_height(ta->label, 21);
}

// Object creators

lv_obj_t* etx_create(const lv_obj_class_t* class_p, lv_obj_t* parent)
{
  lv_obj_t* obj = lv_obj_class_create_obj(class_p, parent);
  lv_obj_class_init_obj(obj);

  return obj;
}
