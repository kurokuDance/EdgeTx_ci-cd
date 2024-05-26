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

#include <algorithm>
#include <vector>
#include "opentx.h"
#include "hal/module_port.h"

extern uint8_t g_moduleIdx;

struct LuaScript {
  std::string path;
  std::string label;
  bool operator<(const LuaScript &a) { return label < a.label; }
};

inline bool LuaScript_compare_nocase(LuaScript first, LuaScript second)
{
  return strcasecmp(first.label.c_str(), second.label.c_str()) < 0;
}

bool addRadioTool(uint8_t index, const char * label)
{
  if (index >= menuVerticalOffset) {
    uint8_t lineIndex = index - menuVerticalOffset;
    if (lineIndex < NUM_BODY_LINES) {
      int8_t sub = menuVerticalPosition - HEADER_LINE;
      LcdFlags attr = (sub == index ? INVERS : 0);
      coord_t y = MENU_HEADER_HEIGHT + lineIndex * FH;
      lcdDrawNumber(3, y, index + 1, LEADING0 | LEFT, 2);
      lcdDrawText(3 * FW, y, label, (sub == index ? INVERS : 0));
      if (attr && s_editMode > 0) {
        s_editMode = 0;
        killAllEvents();
        return true;
      }
    }
  }
  return false;
}

void addRadioModuleTool(uint8_t index, const char * label, void (* tool)(event_t), uint8_t module)
{
  if (addRadioTool(index, label)) {
    g_moduleIdx = module;
    pushMenu(tool);
  }
}

#if defined(LUA)
void addRadioScriptTool(std::vector<LuaScript> luaScripts)
{
  uint8_t index = 0;
  for (auto luaScript : luaScripts) {
    if (addRadioTool(index++, luaScript.label.c_str())) {
      char toolPath[FF_MAX_LFN + 1];
      strncpy(toolPath, luaScript.path.c_str(), sizeof(toolPath) - 1);
      *((char *)getBasename(toolPath) - 1) = '\0';
      f_chdir(toolPath);

      luaExec(luaScript.path.c_str());
    }
  }
}

void addRadioScriptTool(uint8_t index, const char * path)
{
  char toolName[RADIO_TOOL_NAME_MAXLEN + 1];

  if (!readToolName(toolName, path)) {
    strAppendFilename(toolName, getBasename(path), RADIO_TOOL_NAME_MAXLEN);
  }

  if (addRadioTool(index, toolName)) {
    char toolPath[FF_MAX_LFN];
    strcpy(toolPath, path);
    *((char *)getBasename(toolPath)-1) = '\0';
    f_chdir(toolPath);
    luaExec(path);
  }
}
#endif

void menuRadioTools(event_t event)
{
  if (event == EVT_ENTRY  || event == EVT_ENTRY_UP) {
    memclear(&reusableBuffer.radioTools, sizeof(reusableBuffer.radioTools));
#if defined(PXX2)
    for (uint8_t module = 0; module < NUM_MODULES; module++) {
      if (isModulePXX2(module) && (module == INTERNAL_MODULE ? modulePortPowered(INTERNAL_MODULE) : modulePortPowered(EXTERNAL_MODULE))) {
        moduleState[module].readModuleInformation(&reusableBuffer.radioTools.modules[module], PXX2_HW_INFO_TX_ID, PXX2_HW_INFO_TX_ID);
      }
    }
#endif
  }

  SIMPLE_MENU(STR_MENUTOOLS, menuTabGeneral, MENU_RADIO_TOOLS, HEADER_LINE + reusableBuffer.radioTools.linesCount);

  uint8_t index = 0;

#if defined(LUA)
  FILINFO fno;
  DIR dir;

  FRESULT res = f_opendir(&dir, SCRIPTS_TOOLS_PATH);
  if (res == FR_OK) {
    std::vector<LuaScript> luaScripts;  // gather and sort before adding to menu

    for (;;) {
      TCHAR path[FF_MAX_LFN + 1] = SCRIPTS_TOOLS_PATH "/";

      res = f_readdir(&dir, &fno);                   /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
      if (fno.fattrib & (AM_DIR|AM_HID|AM_SYS)) continue;  // skip subfolders, hidden files and system files
      if (fno.fname[0] == '.') continue;  /* Ignore UNIX hidden files */

      strcat(path, fno.fname);
      if (isRadioScriptTool(fno.fname)) {
        char toolName[RADIO_TOOL_NAME_MAXLEN + 1] = {0};
        const char *label;
        char *ext = (char *)getFileExtension(path);
        if (readToolName(toolName, path)) {
          label = toolName;
        } else {
          *ext = '\0';
          label = getBasename(path);
        }

        luaScripts.emplace_back(LuaScript{path, label});
      }
    }
    f_closedir(&dir);

    std::sort(luaScripts.begin(), luaScripts.end(), LuaScript_compare_nocase);
    addRadioScriptTool(luaScripts);
    index += luaScripts.size();
  }
#endif

#if defined(INTERNAL_MODULE_PXX2)
  if (isPXX2ModuleOptionAvailable(reusableBuffer.radioTools.modules[INTERNAL_MODULE].information.modelID, MODULE_OPTION_SPECTRUM_ANALYSER))
    addRadioModuleTool(index++, STR_SPECTRUM_ANALYSER_INT, menuRadioSpectrumAnalyser, INTERNAL_MODULE);

  if (isPXX2ModuleOptionAvailable(reusableBuffer.radioTools.modules[INTERNAL_MODULE].information.modelID, MODULE_OPTION_POWER_METER))
    addRadioModuleTool(index++, STR_POWER_METER_INT, menuRadioPowerMeter, INTERNAL_MODULE);
#endif

#if defined(HARDWARE_INTERNAL_MODULE) && defined(MULTIMODULE)
  if (g_eeGeneral.internalModule == MODULE_TYPE_MULTIMODULE)
    addRadioModuleTool(index++, STR_SPECTRUM_ANALYSER_INT, menuRadioSpectrumAnalyser, INTERNAL_MODULE);
#endif

#if defined(HARDWARE_EXTERNAL_MODULE)

#if (defined(PXX2) || defined(MULTIMODULE))
  bool has_spectrum_analyser = false;
#if defined(PXX2)
  if (isPXX2ModuleOptionAvailable(reusableBuffer.radioTools.modules[EXTERNAL_MODULE].information.modelID, MODULE_OPTION_SPECTRUM_ANALYSER))
    has_spectrum_analyser = true;
#endif
#if defined(MULTIMODULE)
  if (isModuleMultimodule(EXTERNAL_MODULE))
    has_spectrum_analyser = true;
#endif
  if (has_spectrum_analyser)
    addRadioModuleTool(index++, STR_SPECTRUM_ANALYSER_EXT, menuRadioSpectrumAnalyser, EXTERNAL_MODULE);
#endif
#if defined(PXX2)
  if (isPXX2ModuleOptionAvailable(reusableBuffer.radioTools.modules[EXTERNAL_MODULE].information.modelID, MODULE_OPTION_POWER_METER))
    addRadioModuleTool(index++, STR_POWER_METER_EXT, menuRadioPowerMeter, EXTERNAL_MODULE);
#endif

#if defined(GHOST)
  if (isModuleGhost(EXTERNAL_MODULE))
    addRadioModuleTool(index++, "Ghost Menu", menuGhostModuleConfig, EXTERNAL_MODULE);
#endif

#endif

  if (index == 0) {
    lcdDrawCenteredText(LCD_H/2, STR_NO_TOOLS);
  }

  reusableBuffer.radioTools.linesCount = index;
}
