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

#include "updatemultiprotocol.h"

UpdateMultiProtocol::UpdateMultiProtocol(QWidget * parent) :
  UpdateInterface(parent, CID_MultiProtocol, tr("Multiprotocol"))
{
  // GitHub REST API default ResultsPerPage = 30
  init(QString(GH_API_REPOS).append("/pascallanger/DIY-Multiprotocol-TX-Module"), "", 100);
}

void UpdateMultiProtocol::assetSettingsInit()
{
  if (!isSettingsIndexValid())
    return;

  g.component[id()].initAllAssets();

  {
  ComponentAssetData &cad = g.component[id()].asset[0];
  cad.desc("scripts");
  cad.processes(UPDFLG_Common_Asset);
  cad.flags(cad.processes() | UPDFLG_CopyStructure);
  cad.filterType(UpdateParameters::UFT_Startswith);
  cad.filter("MultiLuaScripts");
  cad.maxExpected(1);
  }
  {
  ComponentAssetData &cad = g.component[id()].asset[1];
  cad.desc("binaries");
  cad.processes(UPDFLG_Common_Asset &~ UPDFLG_Decompress);
  cad.flags(cad.processes() | UPDFLG_CopyFiles);
  cad.filterType(UpdateParameters::UFT_Pattern);
  cad.filter("^mm-stm-serial-.*\\.bin$");
  cad.destSubDir("FIRMWARE");
  }

  qDebug() << "Asset settings initialised";
}
