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

#include "opentxinterface.h"
#include "opentxeeprom.h"
#include "rlefile.h"
#include "appdata.h"
#include "constants.h"

#include <bitset>
#include <QMessageBox>
#include <QTime>
#include <QUrl>

#include "storage.h"  // does this need to be last include?

#define FILE_TYP_GENERAL 1
#define FILE_TYP_MODEL   2

/// fileId of general file
#define FILE_GENERAL   0
/// convert model number 0..MAX_MODELS-1  int fileId
#define FILE_MODEL(n) (1+n)

using namespace Board;

QList<OpenTxEepromInterface *> opentxEEpromInterfaces;

OpenTxEepromInterface::OpenTxEepromInterface(OpenTxFirmware * firmware):
  EEPROMInterface(firmware->getBoard()),
  efile(new RleFile()),
  firmware(firmware)
{
}

OpenTxEepromInterface::~OpenTxEepromInterface()
{
  delete efile;
}

const char * OpenTxEepromInterface::getName()
{
  switch (board) {
    case BOARD_JUMPER_T12:
      return "EdgeTX for Jumper T12";
    case BOARD_JUMPER_TLITE:
      return "EdgeTX for Jumper T-Lite";
    case BOARD_JUMPER_TLITE_F4:
      return "EdgeTX for Jumper T-Lite (F4 MCU)";
    case BOARD_JUMPER_TPRO:
      return "EdgeTX for Jumper T-Pro";
    case BOARD_JUMPER_TPROV2:
      return "EdgeTX for Jumper T-Pro V2";
    case BOARD_JUMPER_T14:
      return "EdgeTX for Jumper T14";
    case BOARD_JUMPER_T16:
      return "EdgeTX for Jumper T16";
    case BOARD_JUMPER_T18:
      return "EdgeTX for Jumper T18";
    case BOARD_JUMPER_T20:
      return "EdgeTX for Jumper T20";
    case BOARD_JUMPER_T20V2:
      return "EdgeTX for Jumper T20 V2";
    case BOARD_RADIOMASTER_TX16S:
      return "EdgeTX for Radiomaster TX16S";
    case BOARD_RADIOMASTER_TX12:
      return "EdgeTX for Radiomaster TX12";
    case BOARD_RADIOMASTER_TX12_MK2:
      return "EdgeTX for Radiomaster TX12 Mark II";
    case BOARD_RADIOMASTER_ZORRO:
      return "EdgeTX for Radiomaster Zorro";
    case BOARD_RADIOMASTER_BOXER:
      return "EdgeTX for Radiomaster Boxer";
    case BOARD_RADIOMASTER_POCKET:
      return "EdgeTX for Radiomaster Pocket";
    case BOARD_RADIOMASTER_T8:
      return "EdgeTX for Radiomaster T8";
    case BOARD_TARANIS_X9D:
      return "EdgeTX for FrSky Taranis X9D";
    case BOARD_TARANIS_X9DP:
      return "EdgeTX for FrSky Taranis X9D+";
    case BOARD_TARANIS_X9DP_2019:
      return "EdgeTX for FrSky Taranis X9D+ 2019";
    case BOARD_TARANIS_X9E:
      return "EdgeTX for FrSky Taranis X9E";
    case BOARD_TARANIS_X7:
      return "EdgeTX for FrSky Taranis X7";
    case BOARD_TARANIS_X7_ACCESS:
      return "EdgeTX for FrSky Taranis X7 Access";
    case BOARD_TARANIS_X9LITES:
      return "EdgeTX for FrSky Taranis X9-Lite S";
    case BOARD_TARANIS_X9LITE:
      return "EdgeTX for FrSky Taranis X9-Lite";
    case BOARD_TARANIS_XLITE:
      return "EdgeTX for FrSky Taranis X-Lite";
    case BOARD_TARANIS_XLITES:
      return "EdgeTX for FrSky Taranis X-Lite S/Pro";
    case BOARD_SKY9X:
      return "EdgeTX for Sky9x board / 9X";
    case BOARD_9XRPRO:
      return "EdgeTX for 9XR-PRO";
    case BOARD_AR9X:
      return "EdgeTX for ar9x board / 9X";
    case BOARD_HORUS_X12S:
      return "EdgeTX for FrSky Horus";
    case BOARD_X10:
      return "EdgeTX for FrSky X10";
    case BOARD_X10_EXPRESS:
      return "EdgeTX for FrSky X10 Express";
    case BOARD_FLYSKY_NV14:
      return "EdgeTX for FlySky NV14";
    case BOARD_FLYSKY_EL18:
      return "EdgeTX for FlySky EL18";
    case BOARD_FLYSKY_PL18:
      return "EdgeTX for FlySky PL18";
    case BOARD_BETAFPV_LR3PRO:
      return "EdgeTx for BETAFPV LR3PRO";
    case BOARD_IFLIGHT_COMMANDO8:
      return "EdgeTX for iFlight Commando 8";
    default:
      return "Board is unknown to EdgeTX";
  }
}

bool OpenTxEepromInterface::loadRadioSettingsFromRLE(GeneralSettings & settings, RleFile * rleFile, uint8_t version)
{
  // Companion loads the current profile firmware and associated board hardware definition ONLY!!!!
  // For each OpenTxEepromInterface create a firmware variant to force the associated base firmware board hardware definition to be loaded.
  // Note: The base firmware instance does not do load its hardware definition to save unnecessary processing.
  // To create the variant take the base id and append the language from the current radio profile.
  Firmware *fw = Firmware::getFirmwareForId(firmware->getId().append("-%1").arg(g.currentProfile().fwType().split('-').last()));
  if (!fw) {
    qWarning() << " error unable to create firmware variant";
    return false;
  }

  QByteArray data(sizeof(settings), 0); // GeneralSettings should be always bigger than the EEPROM struct
  OpenTxGeneralData open9xSettings(settings, board, version);
  efile->openRd(FILE_GENERAL);
  int size = rleFile->readRlc2((uint8_t *)data.data(), data.size());
  if (size) {
    open9xSettings.Import(data);
    return checkVariant(settings.version, settings.variant);
  }
  else {
    qWarning() << " error when loading general settings";
    return false;
  }
}

bool OpenTxEepromInterface::loadModelFromRLE(ModelData & model, RleFile * rleFile, unsigned int index, uint8_t version, uint32_t variant)
{
  QByteArray data(sizeof(model), 0);  // ModelData should be always bigger than the EEPROM struct
  rleFile->openRd(FILE_MODEL(index));
  int size = rleFile->readRlc2((uint8_t *)data.data(), data.size());
  if (size) {
    if (loadFromByteArray<ModelData, OpenTxModelData>(model, data, version, variant)) {
      model.used = true;
    }
  }
  else {
    model.clear();
  }
  return true;
}

template <class T, class M>
bool OpenTxEepromInterface::saveToByteArray(const T & src, QByteArray & data, uint8_t version)
{
  if (version == 0) {
    version = getLastDataVersion(getBoard());
  }
  QByteArray raw;
  T srcCopy(src); // work on a copy of radio data, because Export() will modify it!
  M manager(srcCopy, board, version, 0);
  // manager.dump();
  manager.Export(raw);
  data.resize(8);
  *((uint32_t*)&data.data()[0]) = Boards::getFourCC(board);
  data[4] = version;
  data[5] = 'M';
  *((uint16_t*)&data.data()[6]) = raw.size();
  data.append(raw);
  return true;
}

template <class T, class M>
bool OpenTxEepromInterface::loadFromByteArray(T & dest, const QByteArray & data, uint8_t version, uint32_t variant)
{
  // Companion loads the current profile firmware and associated board hardware definition ONLY!!!!
  // For each OpenTxEepromInterface create a firmware variant to force the associated base firmware board hardware definition to be loaded.
  // Note: The base firmware instance does not do load its hardware definition to save unnecessary processing.
  // To create the variant take the base id and append the language from the current radio profile.
  Firmware *fw = Firmware::getFirmwareForId(firmware->getId().append("-%1").arg(g.currentProfile().fwType().split('-').last()));
  if (!fw) {
    qWarning() << " error unable to create firmware variant";
    return false;
  }

  M manager(dest, board, version, variant);
  if (manager.Import(data) != 0) {
    return false;
  }
  // manager.dump(); // Dumps the structure so that it's easy to check with firmware datastructs.h
  return true;
}

template <class T, class M>
bool OpenTxEepromInterface::loadFromByteArray(T & dest, const QByteArray & data)
{
  uint32_t fourcc = *((uint32_t*)&data.data()[0]);
  if (Boards::getFourCC(board) != fourcc) {
    if (IS_FAMILY_HORUS_OR_T16(board) && fourcc == 0x3178396F) {
      qDebug() << QString("%1: Deprecated fourcc used %2 vs %3").arg(getName()).arg(fourcc).arg(Boards::getFourCC(board));
    }
    else {
      qDebug() << QString("%1: Wrong fourcc %2 vs %3").arg(getName()).arg(fourcc).arg(Boards::getFourCC(board));
      return false;
    }
  }

  if (Boards::getFourCC(board) == Boards::getFourCC(getCurrentBoard()) &&
      board != getCurrentFirmware()->getBoard()) {
    qDebug() << QString("%1: Fourcc not unique and does not match profile board: %2").arg(getName()).arg(Boards::getBoardName(getCurrentBoard()));
    return false;
  }

  qDebug() << QString("%1: OK").arg(getName());
  uint8_t version = data[4];
  QByteArray raw = data.right(data.size() - 8);
  return loadFromByteArray<T, M>(dest, raw, version);
}

unsigned long OpenTxEepromInterface::load(RadioData &radioData, const uint8_t * eeprom, int size)
{
  QDebug dbg = qDebug();
  dbg.setAutoInsertSpaces(false);
  dbg << "trying " << getName() << " import...";

  std::bitset<NUM_ERRORS> errors;

  if (size != Boards::getEEpromSize(board)) {
    if (size == 4096) {
      int notnull = false;
      for (int i = 2048; i < 4096; i++) {
        if (eeprom[i] != 255) {
          notnull = true;
        }
      }
      if (notnull) {
        dbg << " wrong size (" << size << ")";
        errors.set(WRONG_SIZE);
        return errors.to_ulong();
      }
      else {
        errors.set(HAS_WARNINGS);
        errors.set(WARNING_WRONG_FIRMWARE);
        size = 2048;
      }
    }
    else {
      dbg << " wrong size (" << size << "/" << Boards::getEEpromSize(board) << ")";
      errors.set(WRONG_SIZE);
      return errors.to_ulong();
    }
  }

  if (!efile->EeFsOpen((uint8_t *) eeprom, size, board)) {
    dbg << " wrong file system";
    errors.set(WRONG_FILE_SYSTEM);
    return errors.to_ulong();
  }

  efile->openRd(FILE_GENERAL);

  uint8_t version;
  if (efile->readRlc2(&version, 1) != 1) {
    dbg << " no";
    errors.set(UNKNOWN_ERROR);
    return errors.to_ulong();
  }

  dbg << " version " << (unsigned int) version;

  EepromLoadErrors version_error = checkVersion(version);
  if (version_error == OLD_VERSION) {
    errors.set(version_error);
    errors.set(HAS_WARNINGS);
  }
  else if (version_error == NOT_OPENTX) {
    dbg << " not OpenTX";
    errors.set(version_error);
    return errors.to_ulong();
  }

  if (!loadRadioSettingsFromRLE(radioData.generalSettings, efile, version)) {
    dbg << " ko";
    errors.set(UNKNOWN_ERROR);
    return errors.to_ulong();
  }

  dbg << " variant " << radioData.generalSettings.variant;
  if (getCurrentFirmware()->getCapability(Models) == 0) {
    radioData.models.resize(firmware->getCapability(Models));
  }
  for (int i = 0; i < firmware->getCapability(Models); i++) {
    if (i < (int)radioData.models.size() && !loadModelFromRLE(radioData.models[i], efile, i, version, radioData.generalSettings.variant)) {
      dbg << " ko";
      errors.set(UNKNOWN_ERROR);
      if (getCurrentFirmware()->getCapability(Models) == 0) {
        radioData.models.resize(i);
      }
      return errors.to_ulong();
    }
  }
  dbg << " ok";
  errors.set(ALL_OK);
  return errors.to_ulong();
}

uint8_t OpenTxEepromInterface::getLastDataVersion(Board::Type board)
{
  return 220; // Note: this is the last bin format before the switch to YAML introduced in Companion version 2.6
}

void OpenTxEepromInterface::showErrors(const QString & title, const QStringList & errors)
{
  QString msg;
  if (errors.empty()) {
    msg = tr("Unknown error");
  }
  else {
    int noErrorsToDisplay = std::min((int)errors.size(), 10);
    for (int n = 0; n < noErrorsToDisplay; n++) {
      msg += " -" + errors.at(n) + "\n";
    }
    if (noErrorsToDisplay < errors.size()) {
      msg += tr(" ... plus %1 errors").arg(errors.size() - noErrorsToDisplay) + "\n" + msg;
    }
  }

  QMessageBox::warning(NULL,
                       CPN_STR_TTL_ERROR,
                       title + "\n" + msg);
}

int OpenTxEepromInterface::save(uint8_t * eeprom, const RadioData & radioData, uint8_t version, uint32_t variant)
{
  // TODO QMessageBox::warning should not be called here

  if (version == 0) {
    version = getLastDataVersion(board);
  }

  int size = Boards::getEEpromSize(board);

  efile->EeFsCreate(eeprom, size, board, version);

  if (IS_TARANIS_X9E(board)) {
    variant |= TARANIS_X9E_VARIANT;
  }
  else if (IS_TARANIS_X9LITES(board)) {
    variant |= TARANIS_X9LITES_VARIANT;
  }
  else if (IS_TARANIS_X9LITE(board)) {
    variant |= TARANIS_X9LITE_VARIANT;
  }
  else if (IS_TARANIS_X7_ACCESS(board)) {
    variant |= TARANIS_X7_VARIANT;
  }
  else if (IS_TARANIS_X7(board)) {
    variant |= TARANIS_X7_VARIANT;
  }
  else if (IS_TARANIS_XLITES(board)) {
    variant |= TARANIS_XLITES_VARIANT;
  }
  else if (IS_TARANIS_XLITE(board)) {
    variant |= TARANIS_XLITE_VARIANT;
  }
  else if (IS_JUMPER_T12(board)) {
    variant |= JUMPER_T12_VARIANT;
  }
  else if (IS_JUMPER_TLITE(board)) {
    variant |= JUMPER_TLITE_VARIANT;
  }
  else if (IS_JUMPER_TPRO(board)) {
    variant |= JUMPER_TPRO_VARIANT;
  }
  else if (IS_RADIOMASTER_TX12(board)) {
    variant |= RADIOMASTER_TX12_VARIANT;
  }
  else if (IS_RADIOMASTER_TX12_MK2(board)) {
    variant |= RADIOMASTER_TX12_MK2_VARIANT;
  }
  else if (IS_RADIOMASTER_ZORRO(board)) {
    variant |= RADIOMASTER_ZORRO_VARIANT;
  }
  else if (IS_RADIOMASTER_T8(board)) {
    variant |= RADIOMASTER_T8_VARIANT;
  }
  OpenTxGeneralData generator((GeneralSettings &)radioData.generalSettings, board, version, variant);
  // generator.dump();
  QByteArray data;
  generator.Export(data);
  int sz = efile->writeRlc2(FILE_GENERAL, FILE_TYP_GENERAL, (const uint8_t *)data.constData(), data.size());
  if (sz == 0 || generator.errors().count() > 0) {
    showErrors(tr("Cannot write radio settings"), generator.errors());
    return 0;
  }

  for (int i = 0; i < getCurrentFirmware()->getCapability(Models); i++) {
    if (i < (int)radioData.models.size() && !radioData.models[i].isEmpty()) {
      OpenTxModelData generator((ModelData &)radioData.models[i], board, version, variant);
      // generator.dump();
      QByteArray data;
      generator.Export(data);
      int sz = efile->writeRlc2(FILE_MODEL(i), FILE_TYP_MODEL, (const uint8_t *)data.constData(), data.size());
      if (sz == 0 || generator.errors().count() > 0) {
        showErrors(tr("Cannot write model %1").arg(radioData.models[i].name), generator.errors());
        return 0;
      }
    }
  }

  return size;
}

int OpenTxEepromInterface::getSize(const ModelData &model)
{
  if (IS_SKY9X(board))
    return 0;

  if (model.isEmpty())
    return 0;

  QByteArray tmp(Boards::getEEpromSize(Board::BOARD_UNKNOWN), 0);
  efile->EeFsCreate((uint8_t *) tmp.data(), Boards::getEEpromSize(Board::BOARD_UNKNOWN), board, 255/*version max*/);

  ModelData srcCopy(model); // work on a copy of model data, because Export() may modify it!
  OpenTxModelData open9xModel(srcCopy, board, 255/*version max*/, getCurrentFirmware()->getVariantNumber());

  QByteArray eeprom;
  open9xModel.Export(eeprom);
  int sz = efile->writeRlc2(0, FILE_TYP_MODEL, (const uint8_t *) eeprom.constData(), eeprom.size());
  if (sz != eeprom.size()) {
    return -1;
  }
  return efile->size(0);
}

int OpenTxEepromInterface::getSize(const GeneralSettings &settings)
{
  if (IS_SKY9X(board))
    return 0;

  QByteArray tmp(Boards::getEEpromSize(Board::BOARD_UNKNOWN), 0);
  efile->EeFsCreate((uint8_t *) tmp.data(), Boards::getEEpromSize(Board::BOARD_UNKNOWN), board, 255);

  GeneralSettings srcCopy(settings); // work on a copy of settings data, because Export() may modify it!
  OpenTxGeneralData open9xGeneral(srcCopy, board, 255, getCurrentFirmware()->getVariantNumber());
  // open9xGeneral.dump();

  QByteArray eeprom;
  open9xGeneral.Export(eeprom);
  int sz = efile->writeRlc2(0, FILE_TYP_GENERAL, (const uint8_t *) eeprom.constData(), eeprom.size());
  if (sz != eeprom.size()) {
    return -1;
  }
  return efile->size(0);
}

Firmware * OpenTxFirmware::getFirmwareVariant(const QString &id)
{
  if (id == getId()) {
    return this;
  }
  else if (id.contains(getId() + "-") || (!id.contains("-") && id.contains(getId()))) {
    Firmware * result = new OpenTxFirmware(id, this);
    // TODO result.variant = firmware->getVariant(id);
    return result;
  }
  else {
    return NULL;
  }
}

int OpenTxFirmware::getCapability(::Capability capability)
{
  switch (capability) {
    case Models:
      if (IS_FAMILY_HORUS_OR_T16(board))
        return 0;
      else
        return 60;
    case Imperial:
      return 0;
    case HasModelImage:
      return (board == BOARD_TARANIS_X9D || IS_TARANIS_PLUS(board) || board == BOARD_TARANIS_X9DP_2019 || IS_FAMILY_HORUS_OR_T16(board));
    case ModelImageNameLen:
      return (IS_FAMILY_HORUS_OR_T16(board) ? 14 : 10); //  including extension if saved and <= CPN_MAX_BITMAP_LEN
    case ModelImageKeepExtn:
      return (IS_FAMILY_HORUS_OR_T16(board) ? true : false);
    case HasBeeper:
      return false;
    case HasPxxCountry:
      return 1;
    case HasGeneralUnits:
      return true;
    case HasNegAndSwitches:
      return true;
    case PPMExtCtrl:
      return 1;
    case PPMFrameLength:
      return 40;
    case FlightModes:
      return 9;
    case FlightModesHaveFades:
      return 1;
    case Heli:
      if (IS_HORUS_OR_TARANIS(board))
        return id.contains("noheli") ? 0 : 1;
      else
        return id.contains("heli") ? 1 : 0;
    case Gvars:
      if (IS_HORUS_OR_TARANIS(board))
        return id.contains("nogvars") ? 0 : 9;
      else if (id.contains("gvars"))
        return 9;
      else
        return 0;
    case ModelName:
      return (IS_FAMILY_HORUS_OR_T16(board) ? 15 : (HAS_LARGE_LCD(board) ? 12 : 10));
    case FlightModesName:
      return (IS_HORUS_OR_TARANIS(board) ? 10 : 6);
    case GvarsName:
      return 3;
    case GvarsInCS:
    case HasFAIMode:
      return 1;
    case GvarsAreNamed:
    case GvarsFlightModes:
      return 1;
    case Mixes:
      return 64;
    case OffsetWeight:
      return 500;
    case Timers:
      return 3;
    case TimersName:
      if (HAS_LARGE_LCD(board))
        return 8;
      else
        return 3;
    case PermTimers:
      return true;
    case CustomFunctions:
      return 64;
    case SafetyChannelCustomFunction:
      return id.contains("nooverridech") ? 0 : 1;
    case LogicalSwitches:
      return 64;
    case CustomAndSwitches:
      return getCapability(LogicalSwitches);
    case LogicalSwitchesExt:
      return true;
    case RotaryEncoders:
        return 0;
    case Outputs:
      return 32;
    case NumCurvePoints:
      return 512;
    case VoicesAsNumbers:
      return 0;
    case VoicesMaxLength:
      return (IS_TARANIS_X9(board) ? 8 : 6);
    case MultiLangVoice:
      return 1;
    case SoundPitch:
      return 1;
    case Haptic:
      return board != Board::BOARD_TARANIS_X9D || id.contains("haptic");
    case ModelTrainerEnable:
      if (IS_HORUS_OR_TARANIS(board) && board!=Board::BOARD_TARANIS_XLITE)
        return 1;
      else
        return 0;
    case MaxVolume:
      return 23;
    case MaxContrast:
      if (IS_TARANIS_SMALL(board))
        return 30;
      else
        return 45;
    case MinContrast:
      if (IS_TARANIS_X9(board))
        return 0;
      else
        return 10;
    case HasSoundMixer:
      return 1;
    case ExtraInputs:
      return 1;
    case TrimsRange:
      return 128;
    case ExtendedTrimsRange:
      return 512;
    case Simulation:
      return 1;
    case NumCurves:
      return 32;
    case HasMixerNames:
      return (IS_TARANIS_X9(board) ? 8 : 6);
    case HasExpoNames:
      return (IS_TARANIS_X9(board) ? 8 : 6);
    case HasNoExpo:
      return (IS_HORUS_OR_TARANIS(board) ? false : true);
    case ChannelsName:
      return (HAS_LARGE_LCD(board) ? 6 : 4);
    case HasCvNames:
      return 1;
    case Telemetry:
      return 1;
    case TelemetryBars:
      return 1;
    case TelemetryCustomScreens:
      if (IS_FAMILY_HORUS_OR_T16(board))
        return 0;
      else
        return 4;
    case TelemetryCustomScreensBars:
      return (getCapability(TelemetryCustomScreens) ? 4 : 0);
    case TelemetryCustomScreensLines:
      return (getCapability(TelemetryCustomScreens) ? 4 : 0);
    case TelemetryCustomScreensFieldsPerLine:
      return HAS_LARGE_LCD(board) ? 3 : 2;
    case NoTelemetryProtocol:
      return IS_HORUS_OR_TARANIS(board) ? 1 : 0;
    case TelemetryUnits:
      return 0;
    case TelemetryMaxMultiplier:
      return 32;
    case PPMCenter:
      return (IS_HORUS_OR_TARANIS(board) ? 500 : (id.contains("ppmca") ? 125 : 0));
    case SYMLimits:
      return 1;
    case OptrexDisplay:
      return (board == BOARD_SKY9X ? true : false);
    case HasVario:
      return 1;
    case HasVarioSink:
      return true;
    case HasFailsafe:
      return 32;
    case NumModules:
      return 2;
    case NumFirstUsableModule:
      return (IS_JUMPER_T12(board) && !id.contains("internalmulti") ? 1 : 0);
    case HasModuleR9MFlex:
      return id.contains("flexr9m");
    case HasModuleR9MMini:
      return IS_TARANIS_XLITE(board) && !id.contains("stdr9m");
    case HasPPMStart:
      return true;
    case HastxCurrentCalibration:
      return (IS_SKY9X(board) ? true : false);
    case HasVolume:
      return true;
    case HasBrightness:
      return true;
    case PerModelTimers:
      return 1;
    case SlowScale:
      return 10;
    case SlowRange:
      return 250;
    case CSFunc:
      return 18;
    case HasSDLogs:
      return true;
    case LcdWidth:
      if (IS_FLYSKY_NV14(board) || IS_FLYSKY_EL18(board))
        return 320;
      else if (IS_FLYSKY_PL18(board))
        return 480;
      else if (IS_FAMILY_HORUS_OR_T16(board))
        return 480;
      else if (IS_TARANIS_SMALL(board))
        return 128;
      else if (IS_TARANIS(board))
        return 212;
      else
        return 128;
    case LcdHeight:
      if (IS_FLYSKY_NV14(board) || IS_FLYSKY_EL18(board))
        return 480;
      else if (IS_FLYSKY_PL18(board))
        return 320;
      else if (IS_FAMILY_HORUS_OR_T16(board))
        return 272;
      else
        return 64;
    case LcdDepth:
      if (IS_FAMILY_HORUS_OR_T16(board))
        return 16;
      else if (IS_TARANIS_SMALL(board))
        return 1;
      else if (IS_TARANIS(board))
        return 4;
      else
        return 1;
    case GetThrSwitch:
      return (IS_HORUS_OR_TARANIS(board) ? SWITCH_SF1 : SWITCH_THR);
    case HasDisplayText:
      return 1;
    case HasTopLcd:
      return IS_TARANIS_X9E(board) ? 1 : 0;
    case GlobalFunctions:
      return 64;
    case VirtualInputs:
      return 32;
    case InputsLength:
      return HAS_LARGE_LCD(board) ? 4 : 3;
    case TrainerInputs:
      return 16;
    case LuaScripts:
      return IS_HORUS_OR_TARANIS(board) && id.contains("lua") ? 7 : 0;
    case LuaInputsPerScript:
      return IS_HORUS_OR_TARANIS(board) ? 10 : 0;
    case LuaOutputsPerScript:
      return IS_HORUS_OR_TARANIS(board) ? 6 : 0;
    case LimitsPer1000:
    case EnhancedCurves:
      return 1;
    case HasFasOffset:
      return true;
    case HasMahPersistent:
      return true;
    case SimulatorVariant:
      if (IS_TARANIS_X9E(board))
        return TARANIS_X9E_VARIANT;
      else if (IS_TARANIS_X9LITES(board))
        return TARANIS_X9LITES_VARIANT;
      else if (IS_TARANIS_X9LITE(board))
        return TARANIS_X9LITE_VARIANT;
      else if (IS_TARANIS_X7(board))
        return TARANIS_X7_VARIANT;
      else if (IS_TARANIS_XLITES(board))
        return TARANIS_XLITES_VARIANT;
      else if (IS_TARANIS_XLITE(board))
        return TARANIS_XLITE_VARIANT;
      else if (IS_BETAFPV_LR3PRO(board))
        return BETAFPV_LR3PRO_VARIANT;
      else if (IS_IFLIGHT_COMMANDO8(board))
        return IFLIGHT_COMMANDO8_VARIANT;
      else if (IS_JUMPER_T12(board))
        return JUMPER_T12_VARIANT;
      else if (IS_JUMPER_TLITE(board))
        return JUMPER_TLITE_VARIANT;
      else if (IS_RADIOMASTER_TX12(board))
        return RADIOMASTER_TX12_VARIANT;
      else if (IS_RADIOMASTER_T8(board))
        return RADIOMASTER_T8_VARIANT;
      else
        return 0;
    case MavlinkTelemetry:
      return id.contains("mavlink") ? 1 : 0;
    case SportTelemetry:
      return 1;
    case HasInputDiff:
    case HasMixerExpo:
      return (IS_HORUS_OR_TARANIS(board) ? true : false);
    case HasBatMeterRange:
      return (IS_HORUS_OR_TARANIS(board) ? true : id.contains("battgraph"));
    case DangerousFunctions:
      return id.contains("danger") ? 1 : 0;
    case HasModelLabels:
      return IS_FAMILY_HORUS_OR_T16(board);
    case HasSwitchableJack:
      return IS_TARANIS_XLITES(board);
    case HasSportConnector:
      return IS_ACCESS_RADIO(board, id) || IS_TARANIS_X7(board) || IS_HORUS_X10(board) || IS_TARANIS_XLITE(board);
    case PwrButtonPress:
      return IS_HORUS_OR_TARANIS(board) && (board!=Board::BOARD_TARANIS_X9D) && (board!=Board::BOARD_TARANIS_X9DP);
    case Sensors:
      if (IS_FAMILY_HORUS_OR_T16(board) || IS_TARANIS_X9(board))
        return 60;
      else
        return 40;
    case HasAuxSerialMode:
      return (IS_FAMILY_HORUS_OR_T16(board) && !(IS_FLYSKY_NV14(board) || IS_FLYSKY_EL18(board))) ||
             (IS_TARANIS_X9(board) && !IS_TARANIS_X9DP_2019(board)) ||
             IS_RADIOMASTER_ZORRO(board) || IS_RADIOMASTER_TX12_MK2(board);
    case HasAux2SerialMode:
      return IS_FAMILY_T16(board);
    case HasVCPSerialMode:
      return IS_FAMILY_HORUS_OR_T16(board) || IS_RADIOMASTER_ZORRO(board) ||
             IS_JUMPER_TPRO(board) || IS_RADIOMASTER_TX12_MK2(board) || IS_RADIOMASTER_BOXER(board) || IS_RADIOMASTER_POCKET(board);
    case HasBluetooth:
      return (IS_FAMILY_HORUS_OR_T16(board) || IS_TARANIS_X7(board) || IS_TARANIS_XLITE(board)|| IS_TARANIS_X9E(board) ||
              IS_TARANIS_X9DP_2019(board) || IS_FLYSKY_NV14(board) || IS_FLYSKY_EL18(board) || IS_FLYSKY_PL18(board)) ? true : false;
    case HasADCJitterFilter:
      return IS_HORUS_OR_TARANIS(board);
    case HasTelemetryBaudrate:
      return IS_HORUS_OR_TARANIS(board);
    case TopBarZones:
      return getCapability(LcdWidth) > getCapability(LcdHeight) ? 4 : 2;
    case HasModelsList:
      return IS_FAMILY_HORUS_OR_T16(board);
    case HasFlySkyGimbals:
      return (IS_RADIOMASTER_TX16S(board) && id.contains("flyskygimbals"));
    case RotaryEncoderNavigation:
      return (IS_TARANIS_X9E(board) || IS_TARANIS_X9DP_2019(board) ||
              IS_TARANIS_X7(board) || IS_JUMPER_TPRO(board) ||
              IS_TARANIS_X9LITE(board) || IS_RADIOMASTER_TX12(board) ||
              IS_RADIOMASTER_TX12_MK2(board) || IS_RADIOMASTER_ZORRO(board) ||
              IS_RADIOMASTER_BOXER(board) || IS_RADIOMASTER_TX16S(board) ||
              IS_JUMPER_T18(board) || IS_JUMPER_T20(board) ||
              IS_RADIOMASTER_POCKET(board));
    case HasSoftwareSerialPower:
      return IS_RADIOMASTER_TX16S(board);
    case HasIntModuleMulti:
      return id.contains("internalmulti") || IS_RADIOMASTER_TX16S(board) || IS_JUMPER_T18(board) ||
              IS_RADIOMASTER_TX12(board) || IS_JUMPER_TLITE(board) || IS_BETAFPV_LR3PRO(board) ||
              (IS_RADIOMASTER_ZORRO(board) && !id.contains("internalelrs")) ||
              (IS_RADIOMASTER_BOXER(board) && !id.contains("internalelrs")) ||
              (IS_RADIOMASTER_POCKET(board) && !id.contains("internalelrs"));
    case HasIntModuleCRSF:
      return id.contains("internalcrsf");
    case HasIntModuleELRS:
      return id.contains("internalelrs") || IS_RADIOMASTER_TX12_MK2(board) ||
             IS_IFLIGHT_COMMANDO8(board) || IS_RADIOMASTER_BOXER(board) ||
             IS_RADIOMASTER_POCKET(board) || IS_JUMPER_T20(board);
    case HasIntModuleFlySky:
      return  id.contains("afhds2a") || id.contains("afhds3") ||
              IS_FLYSKY_NV14(board) || IS_FLYSKY_EL18(board);
    case BacklightLevelMin:
      if (IS_HORUS_X12S(board))
        return 5;
      if (IS_FAMILY_T16(board) || IS_FLYSKY_NV14(board) || IS_FLYSKY_EL18(board))
        return 1;
      return 46;
    default:
      return 0;
  }
}

QString OpenTxFirmware::getCapabilityStr(::Capability capability)
{
  switch (capability) {
    case ModelImageFilters:
      return IS_FAMILY_HORUS_OR_T16(board) ? "*.bmp|*.jpg|*.png" : "*.bmp";
    default:
      return QString();
  }
}

QTime OpenTxFirmware::getMaxTimerStart()
{
  if (IS_HORUS_OR_TARANIS(board))
    return QTime(23, 59, 59);
  else
    return QTime(8, 59, 59);
}

template<typename T, size_t SIZE>
size_t getSizeA(T (&)[SIZE])
{
  return SIZE;
}

EepromLoadErrors OpenTxEepromInterface::checkVersion(unsigned int version)
{
  switch (version) {
    case 201:
      // first version
    case 202:
      // channel order is now always RUD - ELE - THR - AIL
      // changes in timers
      // ppmFrameLength added
      // thrTraceSrc added
    case 203:
      // mixers changed (for the trims use for change the offset of a mix)
      // telemetry offset raised to -127 +127
      // function switches now have a param on 4 bits
    case 204:
      // telemetry changes (bars)
    case 205:
      // mixer changes (differential, negative curves)...
      // case 206:
    case 207:
      // V4: Rotary Encoders position in FlightModes
    case 208:
      // Trim value in 16bits
      // FrSky A1/A2 offset on 12bits
      // ARM: More Mixers / Expos / CSW / FSW / CHNOUT
    case 209:
      // Add TrmR, TrmE, TrmT, TrmA as Mix sources
      // Trims are now OFF / ON / Rud / Ele / Thr / Ail
    case 210:
      // Add names in Mixes / Expos
      // Add a new telemetry screen
      // Add support for Play Track <filename>
    case 211:
      // Curves big change
    case 212:
      // Big changes in mixers / limitse
    case 213:
      // GVARS / Variants introduction
    case 214:
      // Massive EEPROM change!
    case 215:
      // M128 revert because too much RAM used!
    case 216:
      // A lot of things (first github release)
    case 217:
      // 3 logical switches removed on M128 / gruvin9x boards
        return OLD_VERSION;
    case 218:
      // ACCESS
      // switches add for X7/X10/X12S
      // 60 sensors for X12
      return OLD_VERSION;

    case 219:
      // 60 (Horus / X9) / 40 (others) telemetry sensors instead of 32
      // ALL: ReceiverData array added
      // ALL: registrationId added
      // ALL: failsafeChannels moved from ModuleData to ModelData
      // ALL: ModuleData / TrainerModuleData modified
      // PCBX9 : 6 chars for expos / mixes names instead of 8
      // on X7: 2 additional switches
      // on X9D / X9D+: 1 additional switch
      // on xlite : 2 more storage switches
      // on X10: 2 additional pots => 12 multipos switches
      return OLD_VERSION;

    case 220:
      // Refer radio/src/storage/conversions/conversions_219_220.cpp
      // ALL: TimerData split mode into separate mode and switch and change modes
      // ALL: TimerData countdownStart invert value
      // ALL: convert more fields from zchar to normal string
      // COLORLCD: change CustomScreenData and TopBarPersistentData
      return OLD_VERSION;

    case 221:
      // ALL: YAML storage
      // X10PCB: add pots EXT3 and EXT4 (conversion required as shifts slider settings in multiple fields)
      // RM TX16S: add Flysky gimbals firmware build option and repurpose EXT3 and EXT4 if selected
      // ??: add source TX GPS
      break;

    default:
      return NOT_OPENTX;
  }

  return ALL_OK;
}

bool OpenTxEepromInterface::checkVariant(unsigned int version, unsigned int variant)
{
  bool variantError = false;
  if (IS_TARANIS_X9E(board)) {
    if (variant != TARANIS_X9E_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_TARANIS_X7(board)) {
    if (variant != TARANIS_X7_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_TARANIS_XLITES(board)) {
    if (variant != TARANIS_XLITES_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_TARANIS_XLITE(board)) {
    if (variant != TARANIS_XLITE_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_TARANIS_X9LITES(board)) {
    if (variant != TARANIS_X9LITES_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_TARANIS_X9LITE(board)) {
    if (variant != TARANIS_X9LITE_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_JUMPER_T12(board)) {
    if (variant != JUMPER_T12_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_JUMPER_TLITE(board)) {
    if (variant != JUMPER_TLITE_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_JUMPER_TPRO(board)) {
    if (variant != JUMPER_TPRO_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_RADIOMASTER_TX12(board)) {
    if (variant != RADIOMASTER_TX12_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_RADIOMASTER_TX12_MK2(board)) {
    if (variant != RADIOMASTER_TX12_MK2_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_RADIOMASTER_ZORRO(board)) {
    if (variant != RADIOMASTER_ZORRO_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_RADIOMASTER_T8(board)) {
    if (variant != RADIOMASTER_T8_VARIANT) {
      variantError = true;
    }
  }
  else if (IS_TARANIS(board)) {
    if (variant != 0) {
      variantError = true;
    }
  }

  if (variantError) {
    qWarning() << " wrong variant (" << variant << ")";
    return false;
  }

  return true;
}

bool
OpenTxEepromInterface::loadModelFromBackup(ModelData & model, const uint8_t * data, unsigned int size, uint8_t version, uint32_t variant)
{
  QByteArray backupData((char *) data, size);
  QByteArray modelData;
  if (IS_SKY9X(board)) {
    modelData = QByteArray((char *) data, size);
  }
  else {
    importRlc(modelData, backupData);
  }
  if (modelData.size()) {
    if (loadFromByteArray<ModelData, OpenTxModelData>(model, modelData, version, variant)) {
      model.used = true;
      return true;
    }
  }
  model.clear();
  return false;
}

unsigned long OpenTxEepromInterface::loadBackup(RadioData &radioData, const uint8_t * eeprom, int esize, int index)
{
  std::bitset<NUM_ERRORS> errors;

  QDebug dbg = qDebug();
  dbg.setAutoInsertSpaces(false);
  dbg << "trying " << getName() << " backup import...";

  if (esize < 8 || memcmp(eeprom, "o9x", 3) != 0) {
    dbg << " no\n";
    errors.set(WRONG_SIZE);
    return errors.to_ulong();
  }

  Type backupBoard = (Type) -1;
  switch (eeprom[3]) {
    case 0x33:
      backupBoard = BOARD_TARANIS_X9D;
      break;
    case 0x32:
      backupBoard = BOARD_SKY9X;
      break;
    default:
      dbg << " unknown board";
      errors.set(UNKNOWN_BOARD);
      return errors.to_ulong();
  }

  if (backupBoard != board) {
    dbg << " not right board";
    errors.set(WRONG_BOARD);
    return errors.to_ulong();
  }

  uint8_t version = eeprom[4];
  uint8_t bcktype = eeprom[5];
  uint16_t size = ((uint16_t) eeprom[7] << 8) + eeprom[6];
  uint16_t variant = ((uint16_t) eeprom[9] << 8) + eeprom[8];

  dbg << " version " << (unsigned int) version << " ";

  EepromLoadErrors version_error = checkVersion(version);
  if (version_error == OLD_VERSION) {
    errors.set(version_error);
    errors.set(HAS_WARNINGS);
  }
  else if (version_error == NOT_OPENTX) {
    dbg << " not open9x";
    errors.set(version_error);
    return errors.to_ulong();
  }

  if (size > esize - 8) {
    dbg << " wrong size";
    errors.set(WRONG_SIZE);
    return errors.to_ulong();
  }

  if (bcktype == 'M') {
    if (!loadModelFromBackup(radioData.models[index], &eeprom[8], size, version, variant)) {
      dbg << " ko";
      errors.set(UNKNOWN_ERROR);
      return errors.to_ulong();
    }
  }
  else {
    dbg << " backup type not supported";
    errors.set(BACKUP_NOT_SUPPORTED);
    return errors.to_ulong();
  }

  dbg << " ok";
  errors.set(ALL_OK);
  return errors.to_ulong();
}

QString OpenTxFirmware::getReleaseNotesUrl()
{
  return QString("%1/downloads").arg(EDGETX_HOME_PAGE_URL);
}

// Firmware registrations
// NOTE: "recognized" build options are defined in /radio/util/fwoptions.py

void registerOpenTxFirmware(OpenTxFirmware * firmware, bool deprecated = false)
{
  OpenTxEepromInterface * eepromInterface = new OpenTxEepromInterface(firmware);
  firmware->setEEpromInterface(eepromInterface);
  opentxEEpromInterfaces.push_back(eepromInterface);
  eepromInterfaces.push_back(eepromInterface);
  if (!deprecated)
    Firmware::addRegisteredFirmware(firmware);
}

void addOpenTxCommonOptions(OpenTxFirmware * firmware)
{
  static const Firmware::OptionsGroup fai_options = {
    Firmware::Option("faichoice", Firmware::tr("Possibility to enable FAI MODE (no telemetry) at field")),
    Firmware::Option("faimode",   Firmware::tr("FAI MODE (no telemetry) always enabled"))
  };
  firmware->addOptionsGroup(fai_options);
  firmware->addOption("nooverridech", Firmware::tr("No OverrideCH functions available"));
}

enum RfOptions {
  NONE = 0,
  EU = 1 << 0,
  FLEX = 1 << 1,
  AFHDS2A = 1 << 2,
  AFHDS3 = 1 << 3,
};

void addOpenTxRfOptions(OpenTxFirmware * firmware, uint8_t options)
{
  static const Firmware::Option opt_eu("eu", Firmware::tr("Removes D8 FrSky protocol support which is not legal for use in the EU on radios sold after Jan 1st, 2015"));
  static const Firmware::Option opt_fl("flexr9m", Firmware::tr("Enable non certified firmwares"));
  static const Firmware::Option opt_afhds2a("afhds2a", Firmware::tr("Enable AFHDS2A support"));
  static const Firmware::Option opt_afhds3("afhds3", Firmware::tr("Enable AFHDS3 support"));


  if ((options & (EU + FLEX)) == EU + FLEX)
    firmware->addOptionsGroup({opt_eu, opt_fl});
  else if ((options & EU) != 0)
    firmware->addOption(opt_eu);
  else if ((options & FLEX) != 0)
    firmware->addOption(opt_fl);
  if ((options & AFHDS2A) != 0)
    firmware->addOption(opt_afhds2a);
  if ((options & AFHDS3) != 0)
    firmware->addOption(opt_afhds3);
}

void addOpenTxFontOptions(OpenTxFirmware * firmware)
{
  firmware->addOption("sqt5font", Firmware::tr("Use alternative SQT5 font"));
}

void addOpenTxFrskyOptions(OpenTxFirmware * firmware)
{
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
}

void addOpenTxTaranisOptions(OpenTxFirmware * firmware)
{
  addOpenTxFrskyOptions(firmware);
  addOpenTxFontOptions(firmware);
}

void addOpenTxArm9xOptions(OpenTxFirmware * firmware, bool dblkeys = true)
{
  addOpenTxCommonOptions(firmware);
  firmware->addOption("heli", Firmware::tr("Enable HELI menu and cyclic mix support"));
  firmware->addOption("gvars", Firmware::tr("Global variables"), GVARS_VARIANT);
  firmware->addOption("potscroll", Firmware::tr("Pots use in menus navigation"));
  firmware->addOption("autosource", Firmware::tr("In model setup menus automatically set source by moving the control"));
  firmware->addOption("autoswitch", Firmware::tr("In model setup menus automatically set switch by moving the control"));
  firmware->addOption("nographics", Firmware::tr("No graphical check boxes and sliders"));
  firmware->addOption("battgraph", Firmware::tr("Battery graph"));
  firmware->addOption("nobold", Firmware::tr("Don't use bold font for highlighting active items"));
  //firmware->addOption("bluetooth", Firmware::tr("Bluetooth interface"));
  if (dblkeys)
    firmware->addOption("dblkeys", Firmware::tr("Enable resetting values by pressing up and down at the same time"));
  addOpenTxFontOptions(firmware);
  addOpenTxRfOptions(firmware, FLEX);
}

// TODO: refactoring OpenTxFirmware constructor - need to check if used elsewhere and how
#define FIRMWAREID(flavour)  std::string(FIRMWARE_ID_PREFIX).append(flavour).c_str()

void registerOpenTxFirmwares()
{
  OpenTxFirmware * firmware;

  /* BETAFPV LR3PRO board */
  firmware = new OpenTxFirmware(FIRMWAREID("lr3pro"), QCoreApplication::translate("Firmware", "BETAFPV LiteRadio3 Pro"), BOARD_BETAFPV_LR3PRO);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FlySky NV14 board */
  firmware = new OpenTxFirmware(FIRMWAREID("nv14"), QCoreApplication::translate("Firmware", "FlySky NV14"), BOARD_FLYSKY_NV14);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("bluetooth", Firmware::tr("Support for bluetooth module"));
  addOpenTxRfOptions(firmware, FLEX + AFHDS2A + AFHDS3);
  registerOpenTxFirmware(firmware);

  /* FlySky EL18 board */
  firmware = new OpenTxFirmware(FIRMWAREID("el18"), QCoreApplication::translate("Firmware", "FlySky EL18"), BOARD_FLYSKY_EL18);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("bluetooth", Firmware::tr("Support for bluetooth module"));
  addOpenTxRfOptions(firmware, FLEX + AFHDS2A + AFHDS3);
  registerOpenTxFirmware(firmware);

  /* FlySky PL18 board */
  firmware = new OpenTxFirmware(FIRMWAREID("pl18"), Firmware::tr("FlySky PL18"), BOARD_FLYSKY_PL18);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("bluetooth", Firmware::tr("Support for bluetooth module"));
  addOpenTxRfOptions(firmware, FLEX + AFHDS3);
  registerOpenTxFirmware(firmware);

  /* FrSky Horus X10 board */
  firmware = new OpenTxFirmware(FIRMWAREID("x10"), Firmware::tr("FrSky Horus X10 / X10S"), BOARD_X10);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("internalaccess", Firmware::tr("Support for ACCESS internal module replacement"));
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX);

  /* FrSky Horus X10 Express board */
  firmware = new OpenTxFirmware(FIRMWAREID("x10express"), Firmware::tr("FrSky Horus X10 Express / X10S Express"), BOARD_X10_EXPRESS, "x10-access");
  addOpenTxFrskyOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FrSky Horus X12 board */
  firmware = new OpenTxFirmware(FIRMWAREID("x12s"), Firmware::tr("FrSky Horus X12S"), BOARD_HORUS_X12S);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("internalaccess", Firmware::tr("Support for ACCESS internal module replacement"));
  firmware->addOption("pcbdev", Firmware::tr("Use ONLY with first DEV pcb version"));
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX);

  /* FrSky Taranis X7 board */
  firmware = new OpenTxFirmware(FIRMWAREID("x7"), Firmware::tr("FrSky Taranis X7 / X7S"), BOARD_TARANIS_X7);
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX);

  /* FrSky Taranis X7 Access board */
  firmware = new OpenTxFirmware(FIRMWAREID("x7access"), Firmware::tr("FrSky Taranis X7 / X7S Access"), BOARD_TARANIS_X7_ACCESS, "x7-access");
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FrSky Taranis X-Lite board */
  firmware = new OpenTxFirmware(FIRMWAREID("xlite"), Firmware::tr("FrSky Taranis X-Lite"), BOARD_TARANIS_XLITE);
  // firmware->addOption("stdr9m", Firmware::tr("Use JR-sized R9M module"));
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX);

  /* FrSky Taranis X-Lite S/PRO board */
  firmware = new OpenTxFirmware(FIRMWAREID("xlites"), Firmware::tr("FrSky Taranis X-Lite S/PRO"), BOARD_TARANIS_XLITES);
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FrSky Taranis X9-Lite board */
  firmware = new OpenTxFirmware(FIRMWAREID("x9lite"), Firmware::tr("FrSky Taranis X9-Lite"), BOARD_TARANIS_X9LITE);
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FrSky Taranis X9-LiteS board */
  firmware = new OpenTxFirmware(FIRMWAREID("x9lites"), Firmware::tr("FrSky Taranis X9-Lite S"), BOARD_TARANIS_X9LITES);
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FrSky Taranis X9D board */
  firmware = new OpenTxFirmware(FIRMWAREID("x9d"), Firmware::tr("FrSky Taranis X9D"), BOARD_TARANIS_X9D);
  firmware->addOption("noras", Firmware::tr("Disable RAS (SWR)"));
  firmware->addOption("haptic", Firmware::tr("Haptic module installed"));
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX + AFHDS2A + AFHDS3);

  /* FrSky Taranis X9D+ board */
  firmware = new OpenTxFirmware(FIRMWAREID("x9d+"), Firmware::tr("FrSky Taranis X9D+"), BOARD_TARANIS_X9DP, "x9dp");
  firmware->addOption("noras", Firmware::tr("Disable RAS (SWR)"));
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX + AFHDS2A + AFHDS3);

  /* FrSky Taranis X9D+ 2019 board */
  firmware = new OpenTxFirmware(FIRMWAREID("x9d+2019"), Firmware::tr("FrSky Taranis X9D+ 2019"), BOARD_TARANIS_X9DP_2019, "x9dp2019");
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* FrSky Taranis X9E board */
  firmware = new OpenTxFirmware(FIRMWAREID("x9e"), Firmware::tr("FrSky Taranis X9E"), BOARD_TARANIS_X9E);
  firmware->addOption("shutdownconfirm", Firmware::tr("Confirmation before radio shutdown"));
  firmware->addOption("horussticks", Firmware::tr("Horus gimbals installed (Hall sensors)"));
  addOpenTxTaranisOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, EU + FLEX);

  /* iFlight Commando8 board */
  firmware = new OpenTxFirmware(FIRMWAREID("commando8"), QCoreApplication::translate("Firmware", "iFlight Commando8"), BOARD_IFLIGHT_COMMANDO8);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, NONE);

  /* Jumper T-Lite board */
  firmware = new OpenTxFirmware(FIRMWAREID("tlite"), QCoreApplication::translate("Firmware", "Jumper T-Lite"), BOARD_JUMPER_TLITE);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Jumper T-Lite F4 board */
  /* No immediate plans for this to be needed now
  firmware = new OpenTxFirmware(FIRMWAREID("tlitef4"), QCoreApplication::translate("Firmware", "Jumper T-Lite (F4 MCU)"), BOARD_JUMPER_TLITE_F4, "tlitef4", "edgetx-tlite");
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);
  */

  /* Jumper T-Pro board */
  firmware = new OpenTxFirmware(FIRMWAREID("tpro"), QCoreApplication::translate("Firmware", "Jumper T-Pro"), BOARD_JUMPER_TPRO);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Jumper T-Pro V2 board */
  firmware = new OpenTxFirmware(FIRMWAREID("tprov2"), QCoreApplication::translate("Firmware", "Jumper T-Pro V2"), BOARD_JUMPER_TPROV2);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Jumper T12 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t12"), QCoreApplication::translate("Firmware", "Jumper T12 / T12 Pro"), BOARD_JUMPER_T12);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  firmware->addOption("internalmulti", Firmware::tr("Support for MULTI internal module"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Jumper T14 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t14"), Firmware::tr("Jumper T14"), BOARD_JUMPER_T14);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("internalelrs", Firmware::tr("Select if internal ELRS module is installed"));
  addOpenTxRfOptions(firmware, NONE);
  registerOpenTxFirmware(firmware);

  /* Jumper T16 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t16"), Firmware::tr("Jumper T16 / T16+ / T16 Pro"), BOARD_JUMPER_T16);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("internalmulti", Firmware::tr("Support for MULTI internal module"));
  firmware->addOption("bluetooth", Firmware::tr("Support for bluetooth module"));
  addOpenTxRfOptions(firmware, FLEX);
  registerOpenTxFirmware(firmware);

  /* Jumper T18 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t18"), Firmware::tr("Jumper T18"), BOARD_JUMPER_T18);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("bluetooth", Firmware::tr("Support for bluetooth module"));
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Jumper T20 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t20"), Firmware::tr("Jumper T20"), BOARD_JUMPER_T20);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("internalelrs", Firmware::tr("Select if internal ELRS module is installed"));
  addOpenTxRfOptions(firmware, NONE);
  registerOpenTxFirmware(firmware);

  /* Jumper T20 V2 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t20v2"), Firmware::tr("Jumper T20 V2"), BOARD_JUMPER_T20V2);
  addOpenTxFrskyOptions(firmware);
  firmware->addOption("internalelrs", Firmware::tr("Select if internal ELRS module is installed"));
  addOpenTxRfOptions(firmware, NONE);
  registerOpenTxFirmware(firmware);

  /* Radiomaster Boxer board */
  firmware = new OpenTxFirmware(FIRMWAREID("boxer"), QCoreApplication::translate("Firmware", "Radiomaster Boxer"), Board::BOARD_RADIOMASTER_BOXER);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX + AFHDS2A + AFHDS3);

  /* Radiomaster Pocket board */
  firmware = new OpenTxFirmware(FIRMWAREID("pocket"), QCoreApplication::translate("Firmware", "Radiomaster Pocket"), Board::BOARD_RADIOMASTER_POCKET);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX + AFHDS2A + AFHDS3);

  /* Radiomaster T8 board */
  firmware = new OpenTxFirmware(FIRMWAREID("t8"), QCoreApplication::translate("Firmware", "Radiomaster T8"), BOARD_RADIOMASTER_T8);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, NONE);
  firmware->addOption("bindkey", Firmware::tr("Allow bind using bind key"));

  /* Radiomaster TX12 board */
  firmware = new OpenTxFirmware(FIRMWAREID("tx12"), QCoreApplication::translate("Firmware", "Radiomaster TX12"), BOARD_RADIOMASTER_TX12);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Radiomaster TX12 Mark II board */
  firmware = new OpenTxFirmware(FIRMWAREID("tx12mk2"), QCoreApplication::translate("Firmware", "Radiomaster TX12 Mark II"), BOARD_RADIOMASTER_TX12_MK2);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX);

  /* Radiomaster TX16S board */
  firmware = new OpenTxFirmware(FIRMWAREID("tx16s"), Firmware::tr("Radiomaster TX16S / SE / Hall / Masterfire"), BOARD_RADIOMASTER_TX16S);
  addOpenTxFrskyOptions(firmware);
  addOpenTxRfOptions(firmware, FLEX);
  static const Firmware::Option opt_bt("bluetooth", Firmware::tr("Support for bluetooth module"));
  static const Firmware::Option opt_internal_gps("internalgps", Firmware::tr("Support internal GPS"));
  firmware->addOptionsGroup({opt_bt, opt_internal_gps});
  firmware->addOption("flyskygimbals", Firmware::tr("Support hardware mod: FlySky Paladin EV Gimbals"));
  registerOpenTxFirmware(firmware);

  /* Radiomaster Zorro board */
  firmware = new OpenTxFirmware(FIRMWAREID("zorro"), QCoreApplication::translate("Firmware", "Radiomaster Zorro"), Board::BOARD_RADIOMASTER_ZORRO);
  addOpenTxCommonOptions(firmware);
  firmware->addOption("noheli", Firmware::tr("Disable HELI menu and cyclic mix support"));
  firmware->addOption("nogvars", Firmware::tr("Disable Global variables"));
  firmware->addOption("lua", Firmware::tr("Enable Lua custom scripts screen"));
  firmware->addOption("internalelrs", Firmware::tr("Select if internal ELRS module is installed"));
  addOpenTxFontOptions(firmware);
  registerOpenTxFirmware(firmware);
  addOpenTxRfOptions(firmware, FLEX + AFHDS2A + AFHDS3);

//  /* 9XR-Pro */
//  firmware = new OpenTxFirmware(FIRMWAREID("9xrpro"), Firmware::tr("Turnigy 9XR-PRO"), BOARD_9XRPRO);
//  addOpenTxArm9xOptions(firmware, false);
//  registerOpenTxFirmware(firmware, true);
//
//  /* ar9x board */
//  firmware = new OpenTxFirmware(FIRMWAREID("ar9x"), Firmware::tr("9X with AR9X board"), BOARD_AR9X);
//  addOpenTxArm9xOptions(firmware, true);
//  //firmware->addOption("rtc", Firmware::tr("Optional RTC added"));
//  //firmware->addOption("volume", Firmware::tr("i2c volume control added"));
//  registerOpenTxFirmware(firmware, true);
//
//  /* Sky9x board */
//  firmware = new OpenTxFirmware(FIRMWAREID("sky9x"), Firmware::tr("9X with Sky9x board"), BOARD_SKY9X);
//  addOpenTxArm9xOptions(firmware);
//  registerOpenTxFirmware(firmware, true);

  Firmware::sortRegisteredFirmwares();
  Firmware::setDefaultVariant(Firmware::getFirmwareForFlavour("tx16s"));
  Firmware::setCurrentVariant(Firmware::getDefaultVariant());
}

void unregisterOpenTxFirmwares()
{
  foreach (Firmware * f, Firmware::getRegisteredFirmwares()) {
    delete f;
  }
  unregisterEEpromInterfaces();
}

template <class T, class M>
OpenTxEepromInterface * loadFromByteArray(T & dest, const QByteArray & data)
{
  foreach(OpenTxEepromInterface * eepromInterface, opentxEEpromInterfaces) {
    if (eepromInterface->loadFromByteArray<T, M>(dest, data)) {
      return eepromInterface;
    }
  }
  return NULL;
}

template <class T, class M>
bool saveToByteArray(const T & dest, QByteArray & data)
{
  Board::Type board = getCurrentBoard();
  foreach(OpenTxEepromInterface * eepromInterface, opentxEEpromInterfaces) {
    if (eepromInterface->getBoard() == board) {
      return eepromInterface->saveToByteArray<T, M>(dest, data);
    }
  }
  return false;
}

OpenTxEepromInterface * loadModelFromByteArray(ModelData & model, const QByteArray & data)
{
  return loadFromByteArray<ModelData, OpenTxModelData>(model, data);
}

OpenTxEepromInterface * loadRadioSettingsFromByteArray(GeneralSettings & settings, const QByteArray & data)
{
  return loadFromByteArray<GeneralSettings, OpenTxGeneralData>(settings, data);
}

bool writeModelToByteArray(const ModelData & model, QByteArray & data)
{
  return saveToByteArray<ModelData, OpenTxModelData>(model, data);
}

bool writeRadioSettingsToByteArray(const GeneralSettings & settings, QByteArray & data)
{
  return saveToByteArray<GeneralSettings, OpenTxGeneralData>(settings, data);
}
