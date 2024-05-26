/*
 * Copyright (C) OpenTX
 *
 * Based on code named
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

#include "radiodata.h"
#include <QByteArray>

#include <string>
#include <utility>
#include <vector>

constexpr unsigned int CPN_CURRENT_SETTINGS_VERSION = { 221 };

struct EtxModelMetadata {
    std::string filename;
    std::string name;
    int         modelIdx;
};

typedef std::list<EtxModelMetadata> EtxModelfiles;

bool loadLabelsListFromYaml(RadioData::ModelLabels& labels,
                            int& sortOrder,
                            EtxModelfiles& modelFiles,
                            const QByteArray& data);

bool loadModelFromYaml(ModelData& model, const QByteArray& data);
bool loadRadioSettingsFromYaml(GeneralSettings& settings, const QByteArray& data);

bool writeLabelsListToYaml(const RadioData &radioData, QByteArray& data);

bool writeModelToYaml(const ModelData& model, QByteArray& data);
bool writeRadioSettingsToYaml(const GeneralSettings& settings, QByteArray& data);

std::string patchFilenameToYaml(const std::string& str);
