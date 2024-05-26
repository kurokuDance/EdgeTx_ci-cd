#include "simulateduiwidget.h"
#include "ui_simulateduiwidgetT8.h"

// NOTE: RadioUiAction(NUMBER,...): NUMBER relates to enum EnumKeys in the specific board.h

SimulatedUIWidgetT8::SimulatedUIWidgetT8(SimulatorInterface *simulator, QWidget * parent):
  SimulatedUIWidget(simulator, parent),
  ui(new Ui::SimulatedUIWidgetT8)
{
  RadioUiAction * act;

  ui->setupUi(this);

  // add actions in order of appearance on the help menu

  act = new RadioUiAction(12, QList<int>() << Qt::Key_Up, SIMU_STR_HLP_KEY_UP, SIMU_STR_HLP_ACT_MDL);
  addRadioWidget(ui->rightbuttons->addArea(QRect(25, 40, 35, 20), "T8/right-mdl.png", act));

  act = new RadioUiAction(10, QList<int>() << Qt::Key_Enter << Qt::Key_Return, SIMU_STR_HLP_KEYS_GO_UP, SIMU_STR_HLP_ACT_UP);
  addRadioWidget(ui->rightbuttons->addArea(QRect(25, 90, 35, 20), "T8/right-up.png", act));

  act = new RadioUiAction(11, QList<int>() << Qt::Key_Enter << Qt::Key_Return, SIMU_STR_HLP_KEYS_GO_DN, SIMU_STR_HLP_ACT_DN);
  addRadioWidget(ui->rightbuttons->addArea(QRect(25, 135, 35, 20), "T8/right-dn.png", act));

  act = new RadioUiAction(2, QList<int>() << Qt::Key_Enter << Qt::Key_Return, SIMU_STR_HLP_KEYS_ACTIVATE, SIMU_STR_HLP_ACT_ENT);
  addRadioWidget(ui->rightbuttons->addArea(QRect(25, 180, 35, 20), "T8/right-ent.png", act));

  act = new RadioUiAction(14, QList<int>() << Qt::Key_Left, SIMU_STR_HLP_KEY_LFT, SIMU_STR_HLP_ACT_SYS);
  addRadioWidget(ui->leftbuttons->addArea(QRect(60, 45, 35, 20), "T8/left-sys.png", act));

  act = new RadioUiAction(1, QList<int>() << Qt::Key_Down << Qt::Key_Delete << Qt::Key_Escape << Qt::Key_Backspace, SIMU_STR_HLP_KEYS_EXIT, SIMU_STR_HLP_ACT_EXIT);
  addRadioWidget(ui->leftbuttons->addArea(QRect(60, 85, 35, 20), "T8/left-rtn.png", act));

  act = new RadioUiAction(5, QList<int>() << Qt::Key_PageDown, SIMU_STR_HLP_KEY_PGDN, SIMU_STR_HLP_ACT_PGDN);
  addRadioWidget(ui->leftbuttons->addArea(QRect(60, 135, 35, 20), "T8/left-pagedn.png", act));

  act = new RadioUiAction(4, QList<int>() << Qt::Key_PageUp, SIMU_STR_HLP_KEY_PGUP, SIMU_STR_HLP_ACT_PGUP);
  addRadioWidget(ui->leftbuttons->addArea(QRect(60, 180, 35, 20), "T8/left-pageup.png", act));


  m_backlightColors << QColor(215, 243, 255);  // X7 Blue
  m_backlightColors << QColor(166,247,159);
  m_backlightColors << QColor(247,159,166);
  m_backlightColors << QColor(255,195,151);
  m_backlightColors << QColor(247,242,159);

  setLcd(ui->lcd);

  QString css = "#radioUiWidget {"
                "background-color: qlineargradient(spread:reflect, x1:0, y1:0, x2:0, y2:1,"
                "stop:0 rgba(255, 255, 255, 255),"
                "stop:0.757062 rgba(241, 238, 238, 255),"
                "stop:1 rgba(247, 245, 245, 255));"
                "}";

  QTimer * tim = new QTimer(this);
  tim->setSingleShot(true);
  connect(tim, &QTimer::timeout, [this, css]() {
    emit customStyleRequest(css);
  });
  tim->start(100);
}

SimulatedUIWidgetT8::~SimulatedUIWidgetT8()
{
  delete ui;
}
