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

#include "model_mixes.h"
#include "opentx.h"
#include "mixer_edit.h"
#include "mixes.h"
#include "channel_bar.h"

#include <algorithm>

#define SET_DIRTY()     storageDirty(EE_MODEL)

class MPlexIcon : public Window
{
 public:
  MPlexIcon(Window* parent, uint8_t index) :
    Window(parent, {0, 0, 25, 29}),
    index(index)
    {
      MixData* mix = mixAddress(index);
      EdgeTxIcon n = ICON_MPLEX_ADD;
      if (mix->mltpx == MLTPX_MUL) {
        n = ICON_MPLEX_MULTIPLY;
      } else if (mix->mltpx == MLTPX_REPL) {
        n = ICON_MPLEX_REPLACE;
      }
      icon = new StaticIcon(this, 0, 0, n, COLOR_THEME_SECONDARY1);
      icon->center(width(), height());
    }

  void refresh()
  {
    if (icon) {
      icon->show(lv_obj_get_index(lvobj) != 0);
      MixData* mix = mixAddress(index);
      EdgeTxIcon n = ICON_MPLEX_ADD;
      if (mix->mltpx == MLTPX_MUL) {
        n = ICON_MPLEX_MULTIPLY;
      } else if (mix->mltpx == MLTPX_REPL) {
        n = ICON_MPLEX_REPLACE;
      }
      icon->setIcon(n);
    }
  }

  void setIndex(uint8_t i)
  {
    index = i;
  }

 protected:
  uint8_t index;
  StaticIcon* icon = nullptr;
};

class MixLineButton : public InputMixButtonBase
{
 public:
  MixLineButton(Window* parent, uint8_t index, MPlexIcon* mplex) :
    InputMixButtonBase(parent, index),
    mplex(mplex)
  {
    check(isActive());
  }

  void deleteLater(bool detach = true, bool trash = true) override
  {
    if (mplex) mplex->deleteLater(detach, trash);
    ListLineButton::deleteLater(detach, trash);
  }

  void refresh() override
  {
    const MixData& line = g_model.mixData[index];
    setWeight(line.weight, MIX_WEIGHT_MIN, MIX_WEIGHT_MAX);
    setSource(line.srcRaw);

    char tmp_str[64];
    size_t maxlen = sizeof(tmp_str);

    char *s = tmp_str;
    *s = '\0';

    if (line.name[0]) {
      int cnt = lv_snprintf(s, maxlen, "%.*s ", (int)sizeof(line.name), line.name);
      if ((size_t)cnt >= maxlen) maxlen = 0;
      else { maxlen -= cnt; s += cnt; }
    }

    if (line.swtch || line.curve.value) {
      if (line.swtch) {
        char* sw_pos = getSwitchPositionName(line.swtch);
        int cnt = lv_snprintf(s, maxlen, "%s ", sw_pos);
        if ((size_t)cnt >= maxlen) maxlen = 0;
        else { maxlen -= cnt; s += cnt; }
      }
      if (line.curve.value != 0) {
        getCurveRefString(s, maxlen, line.curve);
        int cnt = strnlen(s, maxlen);
        if ((size_t)cnt >= maxlen) maxlen = 0;
        else { maxlen -= cnt; s += cnt; }
      }
    }
    lv_label_set_text_fmt(opts, "%.*s", (int)sizeof(tmp_str), tmp_str);

    mplex->refresh();

    setFlightModes(line.flightModes);
  }

  void setIndex(uint8_t i) override
  {
    ListLineButton::setIndex(i);
    mplex->setIndex(i);
  }

  void updatePos(coord_t x, coord_t y) override
  {
    setPos(x, y);
    mplex->setPos(x - 28, y);
    mplex->show(y > BTN_H);
  }

  lv_obj_t* mplexLvObj() const { return mplex->getLvObj(); }

  void swapLvglGroup(InputMixButtonBase* line2) override
  {
    MixLineButton* swapWith = (MixLineButton*)line2;

    // Swap elements (focus + line list)
    lv_obj_t* obj1 = getLvObj();
    lv_obj_t* obj2 = swapWith->getLvObj();
    if (lv_obj_get_parent(obj1) == lv_obj_get_parent(obj2)) {
      // same input group: swap obj + focus group
      lv_obj_swap(obj1, obj2);
      lv_obj_swap(mplexLvObj(), swapWith->mplexLvObj());
    } else {
      // different input group: swap only focus group
      lv_group_swap_obj(obj1, obj2);
      lv_group_swap_obj(mplexLvObj(), swapWith->mplexLvObj());
    }
  }

 protected:
  MPlexIcon* mplex = nullptr;

  bool isActive() const override { return isMixActive(index); }
};

class MixGroup : public InputMixGroupBase
{
 public:
  MixGroup(Window* parent, mixsrc_t idx) :
    InputMixGroupBase(parent, idx)
  {
    adjustHeight();

    lv_obj_set_pos(label, 2, -1);

    lv_obj_t* chText = nullptr;
    if (idx >= MIXSRC_FIRST_CH && idx <= MIXSRC_LAST_CH &&
        g_model.limitData[idx - MIXSRC_FIRST_CH].name[0] != '\0') {
      chText = lv_label_create(lvobj);
      etx_font(chText, FONT_XS_INDEX);
      lv_label_set_text_fmt(chText, TR_CH "%" PRIu32,
                            UINT32_C(idx - MIXSRC_FIRST_CH + 1));
      lv_obj_set_pos(chText, 2, 17);
    }

    refresh();
  }

  void enableMixerMonitor()
  {
    if (!monitor)
      monitor = new MixerChannelBar(this, {LCD_W - 118, 1, 100, 14}, idx - MIXSRC_FIRST_CH);
    monitorVisible = true;
    monitor->show();
    adjustHeight();
  }

  void disableMixerMonitor()
  {
    monitorVisible = false;
    monitor->hide();
    adjustHeight();
  }

  void adjustHeight() override
  {
    coord_t y = monitorVisible ? 17 : 2;
    for (auto it = lines.cbegin(); it != lines.cend(); ++it) {
      auto line = *it;
      line->updatePos(LN_X, y);
      y += line->height() + 2;
    }
    setHeight(y + 4);
  }

 protected:
  MixerChannelBar* monitor = nullptr;
  bool monitorVisible = false;
};

ModelMixesPage::ModelMixesPage() : InputMixPageBase(STR_MIXES, ICON_MODEL_MIXER)
{
}

bool ModelMixesPage::reachMixesLimit()
{
  if (getMixCount() >= MAX_MIXERS) {
    new MessageDialog(form, STR_WARNING, STR_NOFREEMIXER);
    return true;
  }
  return false;
}

InputMixGroupBase* ModelMixesPage::getGroupByIndex(uint8_t index)
{
  MixData* mix = mixAddress(index);
  if (is_memclear(mix, sizeof(MixData))) return nullptr;

  int ch = mix->destCh;
  return getGroupBySrc(MIXSRC_FIRST_CH + ch);
}

InputMixGroupBase* ModelMixesPage::createGroup(Window* form, mixsrc_t src)
{
  auto group = new MixGroup(form, src);
  if (showMonitors) group->enableMixerMonitor();
  return group;
}

InputMixButtonBase* ModelMixesPage::createLineButton(InputMixGroupBase *group, uint8_t index)
{
  auto mplex = new MPlexIcon(group, index);
  auto button = new MixLineButton(group, index, mplex);
  button->refresh();

  lines.emplace_back(button);
  group->addLine(button);

  uint8_t ch = group->getMixSrc() - MIXSRC_FIRST_CH;
  button->setPressHandler([=]() -> uint8_t {
    Menu *menu = new Menu(form);
    menu->addLine(STR_EDIT, [=]() {
        uint8_t idx = button->getIndex();
        editMix(ch, idx);
      });
    if (!reachMixesLimit()) {
      if (this->_copyMode != 0) {
        menu->addLine(STR_PASTE_BEFORE, [=]() {
          uint8_t idx = button->getIndex();
          pasteMixBefore(idx);
        });
        menu->addLine(STR_PASTE_AFTER, [=]() {
          uint8_t idx = button->getIndex();
          pasteMixAfter(idx);
        });
      }
      menu->addLine(STR_INSERT_BEFORE, [=]() {
        uint8_t idx = button->getIndex();
        insertMix(ch, idx);
      });
      menu->addLine(STR_INSERT_AFTER, [=]() {
        uint8_t idx = button->getIndex();
        insertMix(ch, idx + 1);
      });
      menu->addLine(STR_COPY, [=]() {
        this->_copyMode = COPY_MODE;
        this->_copySrc = button;
      });
      menu->addLine(STR_MOVE, [=]() {
        this->_copyMode = MOVE_MODE;
        this->_copySrc = button;
      });
    }
    menu->addLine(STR_DELETE, [=]() {
      uint8_t idx = button->getIndex();
      deleteMix(idx);
    });
    return 0;
  });

  return button;
}

void ModelMixesPage::addLineButton(uint8_t index)
{
  MixData* mix = mixAddress(index);
  if (is_memclear(mix, sizeof(MixData))) return;
  int channel = mix->destCh;

  InputMixPageBase::addLineButton(MIXSRC_FIRST_CH + channel, index);
}

void ModelMixesPage::newMix()
{
  Menu* menu = new Menu(Layer::back());
  menu->setTitle(STR_MENU_CHANNELS);

  uint8_t index = 0;
  MixData* line = mixAddress(0);

  // search for unused channels
  for (uint8_t ch = 0; ch < MAX_OUTPUT_CHANNELS; ch++) {
    if (index >= MAX_MIXERS) break;
    bool skip_mix = (ch == 0 && is_memclear(line, sizeof(MixData)));
    if (line->destCh == ch && !skip_mix) {
      while (index < MAX_MIXERS && (line->destCh == ch) && !skip_mix) {
        ++index;
        ++line;
        skip_mix = (ch == 0 && is_memclear(line, sizeof(MixData)));
      }
    } else {
      std::string ch_name(getSourceString(MIXSRC_FIRST_CH + ch));
      menu->addLineBuffered(ch_name.c_str(), [=]() { insertMix(ch, index); });
    }
  }
  menu->updateLines();
}

void ModelMixesPage::editMix(uint8_t channel, uint8_t index)
{
  _copyMode = 0;

  auto line = getLineByIndex(index);
  if (!line) return;

  auto edit = new MixEditWindow(channel, index);
  edit->setCloseHandler([=]() {
    MixData* mix = mixAddress(index);
    if (is_memclear(mix, sizeof(MixData))) {
      deleteMix(index);
    } else {
      line->refresh();
    }
    (getGroupByIndex(index))->adjustHeight();
  });
}

void ModelMixesPage::insertMix(uint8_t channel, uint8_t index)
{
  _copyMode = 0;

  ::insertMix(index, channel);
  InputMixPageBase::addLineButton(MIXSRC_FIRST_CH + channel, index);
  editMix(channel, index);
}

void ModelMixesPage::deleteMix(uint8_t index)
{
  _copyMode = 0;

  auto group = getGroupByIndex(index);
  if (!group) return;

  auto line = getLineByIndex(index);
  if (!line) return;

  ::deleteMix(index);

  group->removeLine(line);
  if (group->getLineCount() == 0) {
    group->deleteLater();
    removeGroup(group);
  } else {
    line->deleteLater();
  }
  removeLine(line);
}

void ModelMixesPage::pasteMix(uint8_t dst_idx, uint8_t channel)
{
  if (!_copyMode || !_copySrc) return;
  uint8_t src_idx = _copySrc->getIndex();

  ::copyMix(src_idx, dst_idx, channel);
  addLineButton(dst_idx);

  if (_copyMode == MOVE_MODE) {
    src_idx = _copySrc->getIndex();
    deleteMix(src_idx);
  }

  _copyMode = 0;
}

static int _mixChnFromIndex(uint8_t index)
{
  MixData* mix = mixAddress(index);
  if (is_memclear(mix, sizeof(MixData))) return -1;
  return mix->destCh;
}

void ModelMixesPage::pasteMixBefore(uint8_t dst_idx)
{
  int channel = _mixChnFromIndex(dst_idx);
  pasteMix(dst_idx, channel);
}

void ModelMixesPage::pasteMixAfter(uint8_t dst_idx)
{
  int channel = _mixChnFromIndex(dst_idx);
  pasteMix(dst_idx + 1, channel);
}

void ModelMixesPage::build(Window * window)
{
  window->setFlexLayout(LV_FLEX_FLOW_COLUMN, 3);

  form = new Window(window, rect_t{});
  form->setFlexLayout(LV_FLEX_FLOW_COLUMN, 3);

  auto box = new Window(window, rect_t{});
  box->padAll(PAD_TINY);
  box->setFlexLayout(LV_FLEX_FLOW_ROW, PAD_SMALL);
  box->padLeft(lv_dpx(8));

  auto box_obj = box->getLvObj();
  lv_obj_set_style_flex_cross_place(box_obj, LV_FLEX_ALIGN_CENTER, 0);

  new StaticText(box, rect_t{}, STR_SHOW_MIXER_MONITORS);
  new ToggleSwitch(
      box, rect_t{}, [=]() { return showMonitors; },
      [=](uint8_t val) { enableMonitors(val); });

  auto btn = new TextButton(window, rect_t{}, LV_SYMBOL_PLUS, [=]() {
    newMix();
    return 0;
  });
  auto btn_obj = btn->getLvObj();
  lv_obj_set_width(btn_obj, lv_pct(100));
  lv_group_focus_obj(btn_obj);

  groups.clear();
  lines.clear();

  bool focusSet = false;
  uint8_t index = 0;
  MixData* line = g_model.mixData;
  for (uint8_t ch = 0; (ch < MAX_OUTPUT_CHANNELS) && (index < MAX_MIXERS); ch++) {

    bool skip_mix = (ch == 0 && is_memclear(line, sizeof(MixData)));
    if (line->destCh == ch && !skip_mix) {

      // one group for the complete mixer channel
      auto group = createGroup(form, MIXSRC_FIRST_CH + ch);
      groups.emplace_back(group);
      while (index < MAX_MIXERS && (line->destCh == ch) && !skip_mix) {
        // one button per input line
        auto btn = createLineButton(group, index);
        if (!focusSet) {
          focusSet = true;
          lv_group_focus_obj(btn->getLvObj());
        }
        ++index;
        ++line;
        skip_mix = (ch == 0 && is_memclear(line, sizeof(MixData)));
      }
    }
  }
}

void ModelMixesPage::enableMonitors(bool enabled)
{
  if (showMonitors == enabled) return;
  showMonitors = enabled;

  for(auto* g : groups) {
    MixGroup* group = (MixGroup*)g;
    if (enabled) {
      group->enableMixerMonitor();
    } else {
      group->disableMixerMonitor();
    }
  }
}
