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

#include "radio_theme.h"

#include "file_carosell.h"
#include "color_editor.h"
#include "color_list.h"
#include "libopenui.h"
#include "opentx.h"
#include "page.h"
#include "preview_window.h"
#include "themes/etx_lv_theme.h"

constexpr int COLOR_PREVIEW_SIZE = 18;

#if LCD_W > LCD_H
constexpr int LIST_WIDTH = ((LCD_W - 12) / 2 - COLOR_PREVIEW_SIZE);
constexpr int COLOR_LIST_WIDTH = ((LCD_W * 3) / 10);
#else
constexpr int LIST_HEIGHT = (LCD_H / 2 - 38);
constexpr int COLOR_LIST_HEIGHT = (LCD_H / 2 - 24);
#endif

#if LCD_W > LCD_H
constexpr int BUTTON_WIDTH = 75;

constexpr int COLOR_BOX_WIDTH = 45;
#else
constexpr int BUTTON_WIDTH = 65;

constexpr int COLOR_BOX_WIDTH = 55;
#endif

constexpr int BUTTON_HEIGHT = 30;
constexpr int COLOR_BOX_HEIGHT = 30;

constexpr int BOX_MARGIN = 2;
constexpr int MAX_BOX_WIDTH = 15;

class ThemeColorPreview : public Window
{
 public:
  ThemeColorPreview(Window *parent, const rect_t &rect,
                    std::vector<ColorEntry> colorList) :
      Window(parent, rect), colorList(colorList)
  {
    setWindowFlag(NO_FOCUS);

    padAll(PAD_ZERO);
#if LCD_W > LCD_H
    setFlexLayout(LV_FLEX_FLOW_COLUMN, BOX_MARGIN);
#else
    setFlexLayout(LV_FLEX_FLOW_ROW, BOX_MARGIN);
#endif
    build();
  }

  void build()
  {
    clear();
    setBoxWidth();
    int size = (boxWidth + BOX_MARGIN) * colorList.size() - BOX_MARGIN;
#if LCD_W > LCD_H
    padTop((height() - size) / 2);
#else
    padLeft((width() - size) / 2);
#endif
    for (auto color : colorList) {
      new ColorSwatch(this, {0, 0, boxWidth, boxWidth}, color.colorValue);
    }
  }

  void setColorList(std::vector<ColorEntry> colorList)
  {
    this->colorList = colorList;
    build();
  }

 protected:
  std::vector<ColorEntry> colorList;
  int boxWidth = MAX_BOX_WIDTH;

  void setBoxWidth()
  {
#if LCD_W > LCD_H
    boxWidth =
        (height() - (colorList.size() - 1) * BOX_MARGIN) / colorList.size();
#else
    boxWidth =
        (width() - (colorList.size() - 1) * BOX_MARGIN) / colorList.size();
#endif
    boxWidth = min(boxWidth, MAX_BOX_WIDTH);
  }
};

static const lv_coord_t d_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(3),
                                       LV_GRID_TEMPLATE_LAST};
static const lv_coord_t b_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                       LV_GRID_TEMPLATE_LAST};
static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

class ThemeDetailsDialog : public BaseDialog
{
 public:
  ThemeDetailsDialog(
      Window *parent, ThemeFile theme,
      std::function<bool(ThemeFile theme)> saveHandler = nullptr) :
      BaseDialog(parent, STR_EDIT_THEME_DETAILS, false, DIALOG_DEFAULT_WIDTH,
                 LV_SIZE_CONTENT),
      theme(theme),
      saveHandler(saveHandler)
  {
    FlexGridLayout grid(d_col_dsc, row_dsc, PAD_TINY);

    auto line = form->newLine(grid);
    line->padAll(PAD_TINY);

    new StaticText(line, rect_t{}, STR_NAME);
    auto te = new TextEdit(line, rect_t{}, this->theme.getName(),
                           SELECTED_THEME_NAME_LEN);
    lv_obj_set_grid_cell(te->getLvObj(), LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    line = form->newLine(grid);
    line->padAll(PAD_TINY);
    line->padTop(2);

    new StaticText(line, rect_t{}, STR_AUTHOR);
    te = new TextEdit(line, rect_t{}, this->theme.getAuthor(), AUTHOR_LENGTH);
    lv_obj_set_grid_cell(te->getLvObj(), LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    FlexGridLayout grid2(b_col_dsc, row_dsc, PAD_TINY);

    line = form->newLine(grid2);
    line->padAll(PAD_TINY);

    new StaticText(line, rect_t{}, STR_DESCRIPTION);
    line = form->newLine(grid2);
    line->padAll(PAD_TINY);
    te = new TextEdit(line, rect_t{}, this->theme.getInfo(), INFO_LENGTH);
    lv_obj_set_grid_cell(te->getLvObj(), LV_GRID_ALIGN_STRETCH, 0, 2,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    line = form->newLine(grid2);
    line->padAll(PAD_TINY);
    line->padTop(10);

    auto button =
        new TextButton(line, rect_t{0, 0, lv_pct(30), 32}, STR_CANCEL, [=]() {
          deleteLater();
          return 0;
        });
    lv_obj_set_grid_cell(button->getLvObj(), LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    button =
        new TextButton(line, rect_t{0, 0, lv_pct(30), 32}, STR_SAVE, [=]() {
          if (saveHandler != nullptr)
            if (!saveHandler(this->theme))
              return 0;
          deleteLater();
          return 0;
        });
    lv_obj_set_grid_cell(button->getLvObj(), LV_GRID_ALIGN_CENTER, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
  }

 protected:
  ThemeFile theme;
  std::function<bool(ThemeFile theme)> saveHandler = nullptr;
};

class ColorEditPage : public Page
{
 public:
  ColorEditPage(ThemeFile *theme, LcdColorIndex indexOfColor,
                std::function<void()> updateHandler = nullptr) :
      Page(ICON_RADIO_EDIT_THEME, PAD_SMALL),
      _updateHandler(std::move(updateHandler)),
      _indexOfColor(indexOfColor),
      _theme(theme)
  {
    buildHead(header);
    buildBody(body);
  }

  void setActiveColorBar(int activeTab)
  {
    if (activeTab >= 0 && _activeTab <= (int)_tabs.size()) {
      _activeTab = activeTab;
      for (auto i = 0; i < (int)_tabs.size(); i++)
        _tabs[i]->check(i == _activeTab);
      _colorEditor->setColorEditorType(_activeTab == 1 ? HSV_COLOR_EDITOR
                                                       : RGB_COLOR_EDITOR);
    }
  }

 protected:
  std::function<void()> _updateHandler;
  LcdColorIndex _indexOfColor;
  ThemeFile *_theme;
  TextButton *_cancelButton;
  ColorEditor *_colorEditor;
  PreviewWindow *_previewWindow = nullptr;
  std::vector<ButtonBase *> _tabs;
  int _activeTab = 0;
  ColorSwatch *_colorSquare = nullptr;
  StaticText *_hexBox = nullptr;

  void deleteLater(bool detach = true, bool trash = true) override
  {
    if (_updateHandler != nullptr) _updateHandler();
    Page::deleteLater(detach, trash);
  }

  void setHexStr(uint32_t rgb)
  {
    if (_hexBox) {
      auto r = GET_RED(rgb), g = GET_GREEN(rgb), b = GET_BLUE(rgb);
      char hexstr[8];
      snprintf(hexstr, sizeof(hexstr), "%02X%02X%02X", (uint16_t)r, (uint16_t)g,
               (uint16_t)b);
      _hexBox->setText(hexstr);
    }
  }

  void buildBody(Window *form)
  {
    form->padAll(PAD_SMALL);
#if LCD_W > LCD_H
    form->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_SMALL);
    rect_t r = {0, 0, COLOR_LIST_WIDTH, form->height() - 8};
#else
    form->setFlexLayout(LV_FLEX_FLOW_COLUMN, PAD_SMALL);
    rect_t r = {0, 0, form->width() - 9, COLOR_LIST_HEIGHT};
#endif

    Window *colForm = new Window(form, r);
    colForm->padAll(PAD_ZERO);
    colForm->setFlexLayout(LV_FLEX_FLOW_COLUMN, PAD_SMALL, r.w);

#if LCD_W > LCD_H
    r.w = form->width() - COLOR_LIST_WIDTH - 12;
#else
    r.h = form->height() - COLOR_LIST_HEIGHT - 12;
#endif
    _previewWindow = new PreviewWindow(form, r, _theme->getColorList());

    r.w = colForm->width();
    r.h = COLOR_BOX_HEIGHT;

    Window *colBoxForm = new Window(colForm, r);
    colBoxForm->padAll(PAD_ZERO);
    colBoxForm->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_TINY);

    r.h = colForm->height() - COLOR_BOX_HEIGHT - 4;

    _colorEditor = new ColorEditor(
        colForm, r, _theme->getColorEntryByIndex(_indexOfColor)->colorValue,
        [=](uint32_t rgb) {
          _theme->setColor(_indexOfColor, rgb);
          if (_colorSquare != nullptr) {
            _colorSquare->setColor(rgb);
          }
          if (_previewWindow != nullptr) {
            _previewWindow->setColorList(_theme->getColorList());
          }
          setHexStr(rgb);
        });
    _colorEditor->setColorEditorType(HSV_COLOR_EDITOR);
    _activeTab = 1;

    r.w = COLOR_BOX_WIDTH;
    r.h = COLOR_BOX_HEIGHT;
    _colorSquare = new ColorSwatch(
        colBoxForm, r, _theme->getColorEntryByIndex(_indexOfColor)->colorValue);

    // hexBox
    r.w = 95;
    _hexBox = new StaticText(colBoxForm, r, "", COLOR_THEME_PRIMARY1 | FONT(L) | RIGHT);
    setHexStr(_theme->getColorEntryByIndex(_indexOfColor)->colorValue);
  }

  void buildHead(PageHeader *window)
  {
    // page title
    header->setTitle(STR_EDIT_COLOR);
    auto t2 =
        header->setTitle2(ThemePersistance::getColorNames()[(int)_indexOfColor]);
#if LCD_H > LCD_W
    etx_font(t2->getLvObj(), FONT_XS_INDEX);
#else
    LV_UNUSED(t2);
#endif

    // page tabs
    rect_t r = {LCD_W - 2 * (BUTTON_WIDTH + 5), 6, BUTTON_WIDTH, BUTTON_HEIGHT};
    _tabs.emplace_back(new TextButton(window, r, "RGB", [=]() {
      setActiveColorBar(0);
      return 1;
    }));
    r.x += (BUTTON_WIDTH + 5);
    _tabs.emplace_back(new TextButton(window, r, "HSV", [=]() {
      setActiveColorBar(1);
      return 1;
    }));
    _tabs[1]->check(true);
  }
};

class ThemeEditPage : public Page
{
 public:
  explicit ThemeEditPage(
      ThemeFile *theme,
      std::function<void(ThemeFile &theme)> saveHandler = nullptr) :
      Page(ICON_RADIO_EDIT_THEME, PAD_SMALL),
      _theme(*theme),
      page(this),
      saveHandler(std::move(saveHandler))
  {
    buildHeader(header);
    buildBody(body);
  }

  void onCancel() override
  {
    if (_dirty) {
      new ConfirmDialog(
          this, STR_SAVE_THEME, _theme.getName(),
          [=]() {
            if (saveHandler != nullptr) {
              saveHandler(_theme);
            }
            deleteLater();
          },
          [=]() { deleteLater(); });
    } else {
      deleteLater();
    }
  }

  void checkEvents() override
  {
    if (!started && _themeName) {
      started = true;
      _themeName->setText(_theme.getName());
    }
    Page::checkEvents();
  }

  void editColorPage()
  {
    auto colorEntry = _cList->getSelectedColor();
    new ColorEditPage(&_theme, colorEntry.colorNumber, [=]() {
      _dirty = true;
      _cList->setColorList(_theme.getColorList());
      _previewWindow->setColorList(_theme.getColorList());
    });
  }

  void buildHeader(Window *window)
  {
    // page title
    header->setTitle(STR_EDIT_THEME);
    _themeName = header->setTitle2(_theme.getName());
#if LCD_H > LCD_W
    etx_font(_themeName->getLvObj(), FONT_XS_INDEX);
#endif

    // save and cancel
    rect_t r = {LCD_W - (BUTTON_WIDTH + 5), 6, BUTTON_WIDTH, BUTTON_HEIGHT};
    new TextButton(window, r, STR_DETAILS, [=]() {
      new ThemeDetailsDialog(page, _theme, [=](ThemeFile t) {
        _theme.setAuthor(t.getAuthor());
        _theme.setInfo(t.getInfo());
        _theme.setName(t.getName());

        // update the theme name
        _themeName->setText(_theme.getName());
        _dirty = true;
        return true;
      });
      return 0;
    });
  }

  void buildBody(Window *form)
  {
    form->padAll(PAD_SMALL);
#if LCD_W > LCD_H
    form->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_SMALL);
    rect_t r = {0, 0, COLOR_LIST_WIDTH, form->height() - 8};
#else
    form->setFlexLayout(LV_FLEX_FLOW_COLUMN, PAD_SMALL);
    rect_t r = {0, 0, form->width() - 8, COLOR_LIST_HEIGHT};
#endif

    _cList = new ColorList(form, r, _theme.getColorList());
    _cList->setLongPressHandler([=]() { editColorPage(); });
    _cList->setPressHandler([=]() { editColorPage(); });

#if LCD_W > LCD_H
    r.w = form->width() - COLOR_LIST_WIDTH - 12;
#else
    r.h = form->height() - COLOR_LIST_HEIGHT - 12;
#endif
    _previewWindow = new PreviewWindow(form, r, _theme.getColorList());
  }

 protected:
  bool _dirty = false;
  bool started = false;
  ThemeFile _theme;
  Page *page;
  std::function<void(ThemeFile &theme)> saveHandler = nullptr;
  PreviewWindow *_previewWindow = nullptr;
  ColorList *_cList = nullptr;
  StaticText *_themeName = nullptr;
};

ThemeSetupPage::ThemeSetupPage(TabsGroup *tabsGroup) :
    PageTab(STR_THEME_EDITOR, ICON_RADIO_EDIT_THEME), tabsGroup(tabsGroup)
{
}

void ThemeSetupPage::setAuthor(ThemeFile *theme)
{
  std::string s("");
  if (theme && (strlen(theme->getAuthor()) > 0)) {
    s = s + "By: " + theme->getAuthor();
  }
  authorText->setText(s);
}

void ThemeSetupPage::setName(ThemeFile *theme)
{
  if (theme && (strlen(theme->getName()) > 0)) {
    nameText->setText(theme->getName());
  } else {
    nameText->setText("");
  }
}

bool isTopWindow(Window *window)
{
  Window *parent = window->getParent();
  if (parent != nullptr) {
    return parent == Layer::back();
  }
  return false;
}

void ThemeSetupPage::checkEvents()
{
  PageTab::checkEvents();

  if (fileCarosell) fileCarosell->pause(!isTopWindow(pageWindow));
}

static void removeAllWhiteSpace(char *str)
{
  int len = strlen(str);
  int origLen = len;
  while (len) {
    if (isspace(str[len])) memmove(str + len, str + len + 1, origLen - len);
    len--;
  }
}

void ThemeSetupPage::displayThemeMenu(Window *window, ThemePersistance *tp)
{
  auto menu = new Menu(listBox, false);

  // you can't activate the active theme
  if (listBox->getSelected() != tp->getThemeIndex()) {
    menu->addLine(STR_ACTIVATE, [=]() {
      auto idx = listBox->getSelected();
      tp->applyTheme(idx);
      tp->setDefaultTheme(idx);
      listBox->setActiveItem(idx);
    });
  }

  // you can't edit the default theme
  if (listBox->getSelected() != 0) {
    menu->addLine(STR_EDIT, [=]() {
      auto themeIdx = listBox->getSelected();
      if (themeIdx < 0) return;

      auto theme = tp->getThemeByIndex(themeIdx);
      if (theme == nullptr) return;

      new ThemeEditPage(theme, [=](ThemeFile &editedTheme) {
        // update cached theme data
        *theme = editedTheme;

        theme->serialize();

        // if the theme info currently displayed
        // were changed, update the UI
        if (themeIdx == currentTheme) {
          setAuthor(theme);
          setName(theme);
          listBox->setName(currentTheme, theme->getName());
          themeColorPreview->setColorList(theme->getColorList());
        }

        // if the active theme changed, re-apply it
        if (themeIdx == tp->getThemeIndex()) {
          // Update saved them name in case it was changed.
          tp->setDefaultTheme(themeIdx);
          theme->applyTheme();
        }
      });
    });
  }

  menu->addLine(STR_DUPLICATE, [=]() {
    ThemeFile newTheme;

    new ThemeDetailsDialog(window, newTheme, [=](ThemeFile theme) {
      if (strlen(theme.getName()) != 0) {
        char name[SELECTED_THEME_NAME_LEN + 20];
        strncpy(name, theme.getName(), SELECTED_THEME_NAME_LEN + 19);
        removeAllWhiteSpace(name);

        // use the selected themes color list to make the new theme
        auto themeIdx = listBox->getSelected();
        if (themeIdx < 0) return true;

        auto selTheme = tp->getThemeByIndex(themeIdx);
        if (selTheme == nullptr) return true;

        for (auto color : selTheme->getColorList())
          theme.setColor(color.colorNumber, color.colorValue);

        if (!tp->createNewTheme(name, theme))
          return false;

        tp->refresh();
        listBox->setNames(tp->getNames());
        listBox->setSelected(currentTheme);
      }
      return true;
    });
  });

  // you can't delete the default theme or the currently active theme
  if (listBox->getSelected() != 0 &&
      listBox->getSelected() != tp->getThemeIndex()) {
    menu->addLine(STR_DELETE, [=]() {
      new ConfirmDialog(
          window, STR_DELETE_THEME,
          tp->getThemeByIndex(listBox->getSelected())->getName(), [=] {
            tp->deleteThemeByIndex(listBox->getSelected());
            listBox->setNames(tp->getNames());
            currentTheme = min<int>(currentTheme, tp->getNames().size() - 1);
            listBox->setSelected(currentTheme);
          });
    });
  }
}

void ThemeSetupPage::setSelected(ThemePersistance *tp)
{
  auto value = listBox->getSelected();
  if (themeColorPreview && authorText && nameText && fileCarosell) {
    ThemeFile *theme = tp->getThemeByIndex(value);
    if (theme) {
      themeColorPreview->setColorList(theme->getColorList());
      setAuthor(theme);
      setName(theme);
      fileCarosell->setFileNames(theme->getThemeImageFileNames());
    }
    currentTheme = value;
  }
}

void ThemeSetupPage::setupListbox(Window *window, rect_t r,
                                  ThemePersistance *tp)
{
  listBox = new ListBox(window, r, tp->getNames());
  etx_scrollbar(listBox->getLvObj());
  listBox->setAutoEdit(true);
  listBox->setSelected(currentTheme);
  listBox->setActiveItem(tp->getThemeIndex());
  listBox->setLongPressHandler([=]() {
    setSelected(tp);
    displayThemeMenu(window, tp);
  });
  listBox->setPressHandler([=]() { setSelected(tp); });
}

void ThemeSetupPage::build(Window *window)
{
  window->padAll(PAD_SMALL);
  pageWindow = window;

#if LCD_W > LCD_H
  window->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_TINY);
#else
  window->setFlexLayout(LV_FLEX_FLOW_COLUMN, PAD_TINY);
#endif

  auto tp = ThemePersistance::instance();
  auto theme = tp->getCurrentTheme();
  currentTheme = tp->getThemeIndex();

  themeColorPreview = nullptr;
  listBox = nullptr;
  fileCarosell = nullptr;
  nameText = nullptr;
  authorText = nullptr;

  // create listbox and setup menus
#if LCD_W > LCD_H
  rect_t r = {0, 0, LIST_WIDTH, window->height() - 8};
#else
  rect_t r = {0, 0, window->width() - 8, LIST_HEIGHT};
#endif
  setupListbox(window, r, tp);

#if LCD_W > LCD_H
  r.w = COLOR_PREVIEW_SIZE;
#else
  r.h = COLOR_PREVIEW_SIZE;
#endif

  // setup ThemeColorPreview()
  auto colorList =
      theme != nullptr ? theme->getColorList() : std::vector<ColorEntry>();
  themeColorPreview = new ThemeColorPreview(window, r, colorList);
  themeColorPreview->setWidth(r.w);

#if LCD_W > LCD_H
  r.w = window->width() - LIST_WIDTH - COLOR_PREVIEW_SIZE - 12;
  r.h = window->height() - 8;
#else
  r.w = window->width() - 8;
  r.h = window->height() - LIST_HEIGHT - COLOR_PREVIEW_SIZE - 12;
#endif

  auto rw = new Window(window, r);
  rw->padAll(PAD_ZERO);
  rw->setFlexLayout(LV_FLEX_FLOW_COLUMN, PAD_TINY, r.w);

  r.h -= 46;

  // setup FileCarosell()
  auto fileNames = theme != nullptr ? theme->getThemeImageFileNames()
                                    : std::vector<std::string>();
  fileCarosell = new FileCarosell(rw, r, fileNames);

  r.h = 20;

  // author and name of theme on right side of screen
  nameText = new StaticText(rw, r, "");
  lv_label_set_long_mode(nameText->getLvObj(), LV_LABEL_LONG_DOT);
  authorText = new StaticText(rw, r, "");
  lv_label_set_long_mode(authorText->getLvObj(), LV_LABEL_LONG_DOT);

  setName(theme);
  setAuthor(theme);
}
