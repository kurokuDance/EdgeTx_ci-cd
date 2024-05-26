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

#include "mixerdialog.h"
#include "ui_mixerdialog.h"
#include "radiodata.h"
#include "filtereditemmodels.h"
#include "helpers.h"
#include "namevalidator.h"

MixerDialog::MixerDialog(QWidget *parent, ModelData & model, MixData * mixdata, GeneralSettings & generalSettings, Firmware * firmware,
                         CompoundItemModelFactory * sharedItemModels) :
  QDialog(parent),
  ui(new Ui::MixerDialog),
  model(model),
  generalSettings(generalSettings),
  firmware(firmware),
  md(mixdata),
  lock(false)
{
  ui->setupUi(this);

  Board::Type board = firmware->getBoard();

  dialogFilteredItemModels = new FilteredItemModelFactory();
  int id;

  QLabel * lb_fp[CPN_MAX_FLIGHT_MODES] = {ui->lb_FP0, ui->lb_FP1, ui->lb_FP2, ui->lb_FP3, ui->lb_FP4, ui->lb_FP5, ui->lb_FP6, ui->lb_FP7, ui->lb_FP8 };
  QCheckBox * tmp[CPN_MAX_FLIGHT_MODES] = {ui->cb_FP0, ui->cb_FP1, ui->cb_FP2, ui->cb_FP3, ui->cb_FP4, ui->cb_FP5, ui->cb_FP6, ui->cb_FP7, ui->cb_FP8 };
  for (int i = 0; i < CPN_MAX_FLIGHT_MODES; i++) {
    cb_fp[i] = tmp[i];
  }

  this->setWindowTitle(tr("DEST -> %1").arg(RawSource(SOURCE_TYPE_CH, md->destCh - 1).toString(&model, &generalSettings)));

  id = dialogFilteredItemModels->registerItemModel(new FilteredItemModel(sharedItemModels->getItemModel(AbstractItemModel::IMID_RawSource),
                                                         (RawSource::InputSourceGroups & ~RawSource::NoneGroup) | RawSource::ScriptsGroup),
                                                   "RawSource");
  ui->sourceCB->setModel(dialogFilteredItemModels->getItemModel(id));
  ui->sourceCB->setCurrentIndex(ui->sourceCB->findData(md->srcRaw.toValue()));
  if (ui->sourceCB->currentIndex() < 0 && md->srcRaw.toValue() == 0)
    ui->sourceCB->setCurrentIndex(ui->sourceCB->count() / 2); // '----' not in list so set to first positive entry

  int limit = firmware->getCapability(OffsetWeight);

  id = dialogFilteredItemModels->registerItemModel(new FilteredItemModel(sharedItemModels->getItemModel(AbstractItemModel::IMID_GVarRef)), "GVarRef");

  gvWeightGroup = new GVarGroup(ui->weightGV, ui->weightSB, ui->weightCB, md->weight, model, 100, -limit, limit, 1.0,
                                dialogFilteredItemModels->getItemModel(id));

  gvOffsetGroup = new GVarGroup(ui->offsetGV, ui->offsetSB, ui->offsetCB, md->sOffset, model, 0, -limit, limit, 1.0,
                                dialogFilteredItemModels->getItemModel(id));

  curveRefFilteredItemModels = new CurveRefFilteredFactory(sharedItemModels,
                                                           firmware->getCapability(HasMixerExpo) ? 0 : FilteredItemModel::PositiveFilter);

  curveGroup = new CurveReferenceUIManager(ui->curveTypeCB, ui->curveGVarCB, ui->curveValueSB, ui->curveValueCB, ui->curveImage, md->curve,
                                           model, sharedItemModels, curveRefFilteredItemModels, this);

  connect(curveGroup, &CurveReferenceUIManager::resized, this, [=] () {
          this->adjustSize();
          this->adjustSize(); // second call seems to be required when hidden fields otherwise not all padding removed
  });

  ui->MixDR_CB->setChecked(md->noExpo == 0);

  if (!firmware->getCapability(HasNoExpo)) {
    ui->MixDR_CB->hide();
    ui->label_MixDR->hide();
  }

  if (!firmware->getCapability(VirtualInputs)) {
    for(int i = 0; i < CPN_MAX_STICKS; i++) {
      ui->trimCB->addItem(Boards::getInputName(i, board));
    }
  }

  ui->trimCB->setCurrentIndex(1 - md->carryTrim);

  int namelength = firmware->getCapability(HasMixerNames);
  if (!namelength) {
    ui->label_name->hide();
    ui->mixerName->hide();
  }
  else {
    ui->mixerName->setMaxLength(namelength);
  }
  ui->mixerName->setValidator(new NameValidator(board, this));
  ui->mixerName->setText(md->name);

  if (!firmware->getCapability(FlightModes)) {
    ui->label_phases->hide();
    for (int i = 0; i < CPN_MAX_FLIGHT_MODES; i++) {
      lb_fp[i]->hide();
      cb_fp[i]->hide();
    }
  }
  else {
    ui->label_phases->setToolTip(tr("Click to access popup menu"));
    ui->label_phases->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->label_phases, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(label_phases_customContextMenuRequested(const QPoint &)));
    int mask = 1;
    for (int i = 0; i < CPN_MAX_FLIGHT_MODES ; i++) {
      if ((md->flightModes & mask) == 0) {
        cb_fp[i]->setChecked(true);
      }
      mask <<= 1;
    }
    for (int i = firmware->getCapability(FlightModes); i < CPN_MAX_FLIGHT_MODES; i++) {
      lb_fp[i]->hide();
      cb_fp[i]->hide();
    }
  }

  id = dialogFilteredItemModels->registerItemModel(new FilteredItemModel(sharedItemModels->getItemModel(AbstractItemModel::IMID_RawSwitch),
                                                                         RawSwitch::MixesContext), "RawSwitch");
  ui->switchesCB->setModel(dialogFilteredItemModels->getItemModel(id));
  ui->switchesCB->setCurrentIndex(ui->switchesCB->findData(md->swtch.toValue()));
  ui->warningCB->setCurrentIndex(md->mixWarn);
  ui->mltpxCB->setCurrentIndex(md->mltpx);
  ui->speedPrecCB->setCurrentIndex(md->speedPrec);

  int scale=firmware->getCapability(SlowScale);
  float range=firmware->getCapability(SlowRange);

  ui->delayDownSB->setMaximum(range / scale);
  ui->delayDownSB->setSingleStep(1.0 / scale);
  ui->delayDownSB->setDecimals((scale == 1 ? 0 : 1));
  ui->delayDownSB->setValue((float)md->delayDown / scale);
  ui->delayUpSB->setMaximum(range / scale);
  ui->delayUpSB->setSingleStep(1.0 / scale);
  ui->delayUpSB->setDecimals((scale == 1 ? 0 : 1));
  ui->delayUpSB->setValue((float)md->delayUp / scale);

  if (md->speedPrec) scale = scale * 10;
  ui->slowDownSB->setMaximum(range / scale);
  ui->slowDownSB->setSingleStep(1.0 / scale);
  ui->slowDownSB->setDecimals((scale == 1 ? 0 : scale == 10 ? 1 : 2));
  ui->slowDownSB->setValue((float)md->speedDown/scale);
  ui->slowUpSB->setMaximum(range / scale);
  ui->slowUpSB->setSingleStep(1.0 / scale);
  ui->slowUpSB->setDecimals((scale == 1 ? 0 : scale == 10 ? 1 : 2));
  ui->slowUpSB->setValue((float)md->speedUp/scale);

  valuesChanged();

  connect(ui->mixerName,SIGNAL(editingFinished()),this,SLOT(valuesChanged()));
  connect(ui->sourceCB,SIGNAL(currentIndexChanged(int)),this,SLOT(valuesChanged()));
  connect(ui->trimCB,SIGNAL(currentIndexChanged(int)),this,SLOT(valuesChanged()));
  connect(ui->MixDR_CB,SIGNAL(toggled(bool)),this,SLOT(valuesChanged()));
  connect(ui->switchesCB,SIGNAL(currentIndexChanged(int)),this,SLOT(valuesChanged()));
  connect(ui->warningCB,SIGNAL(currentIndexChanged(int)),this,SLOT(valuesChanged()));
  connect(ui->mltpxCB,SIGNAL(currentIndexChanged(int)),this,SLOT(valuesChanged()));
  connect(ui->delayDownSB,SIGNAL(editingFinished()),this,SLOT(valuesChanged()));
  connect(ui->delayUpSB,SIGNAL(editingFinished()),this,SLOT(valuesChanged()));
  connect(ui->speedPrecCB,SIGNAL(currentIndexChanged(int)),this,SLOT(valuesChanged()));
  connect(ui->slowDownSB,SIGNAL(editingFinished()),this,SLOT(valuesChanged()));
  connect(ui->slowUpSB,SIGNAL(editingFinished()),this,SLOT(valuesChanged()));

  for (int i=0; i<CPN_MAX_FLIGHT_MODES; i++) {
    connect(cb_fp[i],SIGNAL(toggled(bool)),this,SLOT(valuesChanged()));
  }

  adjustSize();
}

MixerDialog::~MixerDialog()
{
  delete ui;
  delete gvWeightGroup;
  delete gvOffsetGroup;
  delete dialogFilteredItemModels;
}

void MixerDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);

  switch (e->type()) {
    case QEvent::LanguageChange:
      ui->retranslateUi(this);
      break;
    default:
      break;
  }
}

void MixerDialog::valuesChanged()
{
  if (!lock) {
    lock = true;
    md->srcRaw  = RawSource(ui->sourceCB->itemData(ui->sourceCB->currentIndex()).toInt());
    if (firmware->getCapability(HasNoExpo)) {
      bool drVisible = (md->srcRaw.type == SOURCE_TYPE_INPUT && md->srcRaw.index < CPN_MAX_STICKS);
      ui->MixDR_CB->setEnabled(drVisible);
      ui->label_MixDR->setEnabled(drVisible);
    }
    md->carryTrim = -(ui->trimCB->currentIndex() - 1);
    md->noExpo    = ui->MixDR_CB->checkState() ? 0 : 1;
    md->swtch     = RawSwitch(ui->switchesCB->itemData(ui->switchesCB->currentIndex()).toInt());
    md->mixWarn   = ui->warningCB->currentIndex();
    md->mltpx     = (MltpxValue)ui->mltpxCB->currentIndex();

    int scale = firmware->getCapability(SlowScale);
    md->delayDown = round(ui->delayDownSB->value() * scale);
    md->delayUp   = round(ui->delayUpSB->value() * scale);

    // Update values based on old precision (in case it changed)
    if (md->speedPrec) scale = scale * 10;
    md->speedDown = round(ui->slowDownSB->value() * scale);
    md->speedUp   = round(ui->slowUpSB->value() * scale);

    // Get new precion and update controls
    md->speedPrec = ui->speedPrecCB->currentIndex();
    float range = firmware->getCapability(SlowRange);
    scale = firmware->getCapability(SlowScale);
    if (md->speedPrec) scale = scale * 10;
    ui->slowDownSB->setMaximum(range / scale);
    ui->slowDownSB->setSingleStep(1.0 / scale);
    ui->slowDownSB->setDecimals((scale == 1 ? 0 : scale == 10 ? 1 : 2));
    ui->slowDownSB->setValue((float)md->speedDown / scale);
    ui->slowUpSB->setMaximum(range / scale);
    ui->slowUpSB->setSingleStep(1.0 / scale);
    ui->slowUpSB->setDecimals((scale == 1 ? 0 : scale == 10 ? 1 : 2));
    ui->slowUpSB->setValue((float)md->speedUp / scale);

    strcpy(md->name, ui->mixerName->text().toLatin1());

    md->flightModes = 0;
    for (int i = CPN_MAX_FLIGHT_MODES - 1; i >= 0 ; i--) {
      if (!cb_fp[i]->checkState()) {
        md->flightModes++;
      }
      md->flightModes <<= 1;
    }
    md->flightModes >>= 1;

    lock = false;
  }
}

void MixerDialog::label_phases_customContextMenuRequested(const QPoint & pos)
{
  QLabel *label = (QLabel *)sender();
  QPoint globalPos = label->mapToGlobal(pos);
  QMenu contextMenu;
  contextMenu.addAction(tr("Clear All"), this, SLOT(fmClearAll()));
  contextMenu.addAction(tr("Set All"), this, SLOT(fmSetAll()));
  contextMenu.addAction(tr("Invert All"), this, SLOT(fmInvertAll()));
  contextMenu.exec(globalPos);
}

void MixerDialog::fmClearAll()
{
  lock = true;
  for (int i = 0; i < CPN_MAX_FLIGHT_MODES; i++) {
    cb_fp[i]->setChecked(false);
  }
  lock = false;
  valuesChanged();
}

void MixerDialog::fmSetAll()
{
  lock = true;
  for (int i = 0; i < CPN_MAX_FLIGHT_MODES; i++) {
    cb_fp[i]->setChecked(true);
  }
  lock = false;
  valuesChanged();
}

void MixerDialog::fmInvertAll()
{
  lock = true;
  for (int i = 0; i < CPN_MAX_FLIGHT_MODES; i++) {
    cb_fp[i]->setChecked(!cb_fp[i]->isChecked());
  }
  lock = false;
  valuesChanged();
}
