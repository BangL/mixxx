/***************************************************************************
                          dlgprefcontrols.cpp  -  description
                             -------------------
    begin                : Sat Jul 5 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QList>
#include <QDir>
#include <QToolTip>
#include <QDoubleSpinBox>
#include <QWidget>
#include <QLocale>
#include <QDesktopWidget>

#include "dlgprefcontrols.h"
#include "configobject.h"
#include "controlobject.h"
#include "controlobjectslave.h"
#include "widget/wnumberpos.h"
#include "engine/enginebuffer.h"
#include "engine/ratecontrol.h"
#include "skin/skinloader.h"
#include "skin/legacyskinparser.h"
#include "playermanager.h"
#include "controlobject.h"
#include "mixxx.h"
#include "defs_urls.h"

DlgPrefControls::DlgPrefControls(QWidget * parent, MixxxMainWindow * mixxx,
                                 SkinLoader* pSkinLoader,
                                 PlayerManager* pPlayerManager,
                                 ConfigObject<ConfigValue> * pConfig)
        :  DlgPreferencePage(parent),
           m_pConfig(pConfig),
           m_mixxx(mixxx),
           m_pSkinLoader(pSkinLoader),
           m_pPlayerManager(pPlayerManager),
           m_iNumConfiguredDecks(0),
           m_iNumConfiguredSamplers(0) {
    setupUi(this);

    m_pNumDecks = new ControlObjectSlave("[Master]", "num_decks", this);
    m_pNumDecks->connectValueChanged(SLOT(slotNumDecksChanged(double)));
    slotNumDecksChanged(m_pNumDecks->get());

    m_pNumSamplers = new ControlObjectSlave("[Master]", "num_samplers", this);
    m_pNumSamplers->connectValueChanged(SLOT(slotNumSamplersChanged(double)));
    slotNumSamplersChanged(m_pNumSamplers->get());

    // Track time display configuration
    m_pControlTrackTimeDisplay = new ControlObject(
            ConfigKey("[Controls]", "ShowDurationRemaining"));
    connect(m_pControlTrackTimeDisplay, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetPositionDisplay(double)));

    // If not present in the config, set the default value
    if (!m_pConfig->exists(ConfigKey("[Controls]","PositionDisplay")))
        m_pConfig->set(ConfigKey("[Controls]","PositionDisplay"),ConfigValue(1));

    if (m_pConfig->getValueString(ConfigKey("[Controls]", "PositionDisplay")).toInt() == 1) {
        radioButtonRemaining->setChecked(true);
        m_pControlTrackTimeDisplay->set(1.0);
    } else {
        radioButtonElapsed->setChecked(true);
        m_pControlTrackTimeDisplay->set(0.0);
    }
    connect(buttonGroupTrackTime, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(slotSetTrackTimeDisplay(QAbstractButton *)));

    // Set default direction as stored in config file
    if (m_pConfig->getValueString(ConfigKey("[Controls]", "RateDir")).length() == 0)
        m_pConfig->set(ConfigKey("[Controls]", "RateDir"),ConfigValue(0));

    connect(checkBoxInvertSpeedSlider, SIGNAL(toggled(bool)), 
            this, SLOT(slotSetRateDir(bool)));

    // Set default range as stored in config file
    if (m_pConfig->getValueString(ConfigKey("[Controls]", "RateRange")).length() == 0)
        m_pConfig->set(ConfigKey("[Controls]", "RateRange"),ConfigValue(2));

    ComboBoxRateRange->clear();
    ComboBoxRateRange->addItem(tr("6%"));
    ComboBoxRateRange->addItem(tr("8% (Technics SL-1210)"));
    ComboBoxRateRange->addItem(tr("10%"));
    ComboBoxRateRange->addItem(tr("20%"));
    ComboBoxRateRange->addItem(tr("30%"));
    ComboBoxRateRange->addItem(tr("40%"));
    ComboBoxRateRange->addItem(tr("50%"));
    ComboBoxRateRange->addItem(tr("60%"));
    ComboBoxRateRange->addItem(tr("70%"));
    ComboBoxRateRange->addItem(tr("80%"));
    ComboBoxRateRange->addItem(tr("90%"));
    connect(ComboBoxRateRange, SIGNAL(activated(int)),
            this, SLOT(slotSetRateRange(int)));

    //
    // Key lock mode
    //
    connect(buttonGroupKeyLockMode, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(slotKeyLockMode(QAbstractButton *)));
    m_keylockMode = m_pConfig->getValueString(
        ConfigKey("[Controls]", "PitchAndKeylockMode"), "0").toInt();
    foreach (ControlObjectThread* pControl, m_keylockModeControls) {
        pControl->slotSet(m_keylockMode);
    }

    //
    // Rate buttons configuration
    //
    //NOTE: THESE DEFAULTS ARE A LIE! You'll need to hack the same values into the static variables
    //      at the top of enginebuffer.cpp
    if (m_pConfig->getValueString(ConfigKey("[Controls]", "RateTempLeft")).length() == 0)
        m_pConfig->set(ConfigKey("[Controls]", "RateTempLeft"), ConfigValue(QString("4.0")));
    if (m_pConfig->getValueString(ConfigKey("[Controls]", "RateTempRight")).length() == 0)
        m_pConfig->set(ConfigKey("[Controls]", "RateTempRight"), ConfigValue(QString("2.0")));
    if (m_pConfig->getValueString(ConfigKey("[Controls]", "RatePermLeft")).length() == 0)
        m_pConfig->set(ConfigKey("[Controls]", "RatePermLeft"), ConfigValue(QString("0.50")));
    if (m_pConfig->getValueString(ConfigKey("[Controls]", "RatePermRight")).length() == 0)
        m_pConfig->set(ConfigKey("[Controls]", "RatePermRight"), ConfigValue(QString("0.05")));

    connect(spinBoxTempRateLeft, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetRateTempLeft(double)));
    connect(spinBoxTempRateRight, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetRateTempRight(double)));
    connect(spinBoxPermRateLeft, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetRatePermLeft(double)));
    connect(spinBoxPermRateRight, SIGNAL(valueChanged(double)),
            this, SLOT(slotSetRatePermRight(double)));

    spinBoxTempRateLeft->setValue(m_pConfig->getValueString(
            ConfigKey("[Controls]", "RateTempLeft")).toDouble());
    spinBoxTempRateRight->setValue(m_pConfig->getValueString(
            ConfigKey("[Controls]", "RateTempRight")).toDouble());
    spinBoxPermRateLeft->setValue(m_pConfig->getValueString(
            ConfigKey("[Controls]", "RatePermLeft")).toDouble());
    spinBoxPermRateRight->setValue(m_pConfig->getValueString(
            ConfigKey("[Controls]", "RatePermRight")).toDouble());

    SliderRateRampSensitivity->setEnabled(true);
    SpinBoxRateRampSensitivity->setEnabled(true);


    //
    // Override Playing Track on Track Load
    //
    // The check box reflects the opposite of the config value
    checkBoxDisallowLoadToPlayingDeck->setChecked(
        m_pConfig->getValueString(ConfigKey("[Controls]", "AllowTrackLoadToPlayingDeck")).toInt()==0);
    connect(checkBoxDisallowLoadToPlayingDeck, SIGNAL(toggled(bool)),
            this, SLOT(slotSetAllowTrackLoadToPlayingDeck(bool)));

    //
    // Locale setting
    //

    // Iterate through the available locales and add them to the combobox
    // Borrowed following snippet from http://qt-project.org/wiki/How_to_create_a_multi_language_application
    QString translationsFolder = m_pConfig->getResourcePath() + "translations/";
    QString currentLocale = pConfig->getValueString(ConfigKey("[Config]", "Locale"));

    QDir translationsDir(translationsFolder);
    QStringList fileNames = translationsDir.entryList(QStringList("mixxx_*.qm"));
    fileNames.push_back("mixxx_en_US.qm"); // add source language as a fake value

    bool indexFlag = false; // it'll indicate if the selected index changed.
    for (int i = 0; i < fileNames.size(); ++i) {
        // Extract locale from filename
        QString locale = fileNames[i];
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.indexOf('_') + 1);
        QLocale qlocale = QLocale(locale);

        QString lang = QLocale::languageToString(qlocale.language());
        QString country = QLocale::countryToString(qlocale.country());
        if (lang == "C") { // Ugly hack to remove the non-resolving locales
            continue;
        }
        lang = QString("%1 (%2)").arg(lang).arg(country);
        ComboBoxLocale->addItem(lang, locale); // locale as userdata (for storing to config)
        if (locale == currentLocale) { // Set the currently selected locale
            ComboBoxLocale->setCurrentIndex(ComboBoxLocale->count() - 1);
            indexFlag = true;
        }
    }
    ComboBoxLocale->model()->sort(0); // Sort languages list

    ComboBoxLocale->insertItem(0, "System", ""); // System default locale - insert at the top
    if (!indexFlag) { // if selectedIndex didn't change - select system default
        ComboBoxLocale->setCurrentIndex(0);
    }
    connect(ComboBoxLocale, SIGNAL(activated(int)),
            this, SLOT(slotSetLocale(int)));

    //
    // Cue Mode
    //

    // Add "(?)" with a manual link to the label
    labelCueMode->setText(
            labelCueMode->text() +
            " <a href=\"" +
            MIXXX_MANUAL_URL +
            "/chapters/user_interface.html#using-cue-modes\">(?)</a>");

    // Set default value in config file and control objects, if not present
    // Default is "0" = Mixxx Mode
    QString cueDefault = m_pConfig->getValueString(ConfigKey("[Controls]", "CueDefault"), "0");
    int cueDefaultValue = cueDefault.toInt();

    // Update combo box
    ComboBoxCueDefault->addItem(tr("Mixxx mode"));
    ComboBoxCueDefault->addItem(tr("Pioneer mode"));
    ComboBoxCueDefault->addItem(tr("Denon mode"));
    ComboBoxCueDefault->addItem(tr("Numark mode"));
    ComboBoxCueDefault->setCurrentIndex(cueDefaultValue);

    slotSetCueDefault(cueDefaultValue);
    connect(ComboBoxCueDefault,   SIGNAL(activated(int)), this, SLOT(slotSetCueDefault(int)));

    // Cue recall
    checkBoxSeekToCue->setChecked(m_pConfig->getValueString(
        ConfigKey("[Controls]", "CueRecall")).toInt()==0);
    //NOTE: for CueRecall, 0 means ON...
    connect(checkBoxSeekToCue, SIGNAL(toggled(bool)),
            this, SLOT(slotSetCueRecall(bool)));

    //
    // Skin configurations
    //
    QString warningString = "<img src=\":/images/preferences/ic_preferences_warning.png\") width=16 height=16 />"
        + tr("The minimum size of the selected skin is bigger than your screen resolution.");
    warningLabel->setText(warningString);

    ComboBoxSkinconf->clear();

    QList<QDir> skinSearchPaths = m_pSkinLoader->getSkinSearchPaths();
    QList<QFileInfo> skins;
    foreach (QDir dir, skinSearchPaths) {
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        skins.append(dir.entryInfoList());
    }

    QString configuredSkinPath = m_pSkinLoader->getSkinPath();
    QIcon sizeWarningIcon(":/images/preferences/ic_preferences_warning.png");
    int index = 0;
    foreach (QFileInfo skinInfo, skins) {
        bool size_ok = checkSkinResolution(skinInfo.absoluteFilePath());
        if (size_ok) {
            ComboBoxSkinconf->insertItem(index, skinInfo.fileName());
        } else {
            ComboBoxSkinconf->insertItem(index, sizeWarningIcon, skinInfo.fileName());
        }

        if (skinInfo.absoluteFilePath() == configuredSkinPath) {
            ComboBoxSkinconf->setCurrentIndex(index);
            if (size_ok) {
                warningLabel->hide();
            } else {
                warningLabel->show();
            }
        }
        index++;
    }

    connect(ComboBoxSkinconf, SIGNAL(activated(int)), this, SLOT(slotSetSkin(int)));
    connect(ComboBoxSchemeconf, SIGNAL(activated(int)), this, SLOT(slotSetScheme(int)));

    slotUpdateSchemes();

    //
    // Start in fullscreen mode
    //
    checkBoxStartFullScreen->setChecked(m_pConfig->getValueString(
                       ConfigKey("[Config]", "StartInFullscreen")).toInt()==1);
    connect(checkBoxStartFullScreen, SIGNAL(toggled(bool)),
            this, SLOT(slotSetStartInFullScreen(bool)));
    //
    // Tooltip configuration
    //

    // Initialize checkboxes to match config
    int configTooltips = m_mixxx->getToolTipsCfg();
    //0=OFF, 1=ON, 2=ON (only in Library)
    switch (configTooltips) {
        case 0: // Off
            checkBoxTooltipsEnabled->setChecked(false);
            break;
        case 1: // On
            checkBoxTooltipsEnabled->setChecked(true);
            checkBoxTooltipsOnlyLibrary->setChecked(false);
            break;
        case 2: // Only in library
            checkBoxTooltipsEnabled->setChecked(true);
            checkBoxTooltipsOnlyLibrary->setChecked(true);
            break;
    }

    slotSetTooltips();  // Update disabled status of "only library" checkbox
    connect(buttonGroupTooltips, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(slotSetTooltips()));

    //
    // Ramping Temporary Rate Change configuration
    //

    // Set Ramp Rate On or Off
    connect(groupBoxRateRamp, SIGNAL(toggled(bool)),
            this, SLOT(slotSetRateRamp(bool)));
    groupBoxRateRamp->setChecked((bool)
                                 m_pConfig->getValueString(ConfigKey("[Controls]", "RateRamp")).toInt());

    // Update Ramp Rate Sensitivity
    connect(SliderRateRampSensitivity, SIGNAL(valueChanged(int)),
            this, SLOT(slotSetRateRampSensitivity(int)));
    SliderRateRampSensitivity->setValue(m_pConfig->getValueString(
            ConfigKey("[Controls]", "RateRampSensitivity")).toInt());

    // Update "reset speed and pitch" check box
    m_speedAutoReset = static_cast<bool>(m_pConfig->getValueString(
                    ConfigKey("[Controls]", "SpeedAutoReset")).toInt());
    checkBoxResetSpeedAndPitch->setChecked(m_speedAutoReset==1);
    connect(checkBoxResetSpeedAndPitch, SIGNAL(toggled(bool)),
            this, SLOT(slotUpdateSpeedAutoReset(bool)));

    slotUpdate();
}

DlgPrefControls::~DlgPrefControls() {
    delete m_pControlTrackTimeDisplay;
    qDeleteAll(m_rateControls);
    qDeleteAll(m_rateDirControls);
    qDeleteAll(m_cueControls);
    qDeleteAll(m_rateRangeControls);
    qDeleteAll(m_keylockModeControls);
}

void DlgPrefControls::slotUpdateSchemes() {
    // Since this involves opening a file we won't do this as part of regular slotUpdate
    QList<QString> schlist = LegacySkinParser::getSchemeList(
                m_pSkinLoader->getSkinPath());

    ComboBoxSchemeconf->clear();

    if (schlist.size() == 0) {
        ComboBoxSchemeconf->setEnabled(false);
        ComboBoxSchemeconf->addItem(tr("This skin does not support color schemes", 0));
        ComboBoxSchemeconf->setCurrentIndex(0);
    } else {
        ComboBoxSchemeconf->setEnabled(true);
        QString selectedScheme = m_pConfig->getValueString(ConfigKey("[Config]", "Scheme"));
        for (int i = 0; i < schlist.size(); i++) {
            ComboBoxSchemeconf->addItem(schlist[i]);

            if (schlist[i] == selectedScheme) {
                ComboBoxSchemeconf->setCurrentIndex(i);
            }
        }
    }
}

void DlgPrefControls::slotUpdate() {
    double deck1RateRange = m_rateRangeControls[0]->get();
    double deck1RateDir = m_rateDirControls[0]->get();

    double idx = (10. * deck1RateRange) + 1;
    if (deck1RateRange <= 0.07)
        idx = 0.;
    else if (deck1RateRange <= 0.09)
        idx = 1.;

    ComboBoxRateRange->setCurrentIndex((int)idx);

    if (deck1RateDir == 1)
        checkBoxInvertSpeedSlider->setChecked(false);
    else
        checkBoxInvertSpeedSlider->setChecked(true);

    if (m_keylockMode == 1)
        radioButtonCurrentKey->setChecked(true);
    else
        radioButtonOriginalKey->setChecked(true);
    
    checkBoxResetSpeedAndPitch->setChecked(m_speedAutoReset==1);
}

void DlgPrefControls::slotResetToDefaults() {
    // Track time display mode
    radioButtonRemaining->setChecked(true);

    // Up increases speed.
    checkBoxInvertSpeedSlider->setChecked(false);

    // 10% Rate Range
    ComboBoxRateRange->setCurrentIndex(2);

    // Don't load tracks into playing decks.
    checkBoxDisallowLoadToPlayingDeck->setChecked(true);

    // Use System locale
    ComboBoxLocale->setCurrentIndex(0);

    // Mixxx cue mode
    ComboBoxCueDefault->setCurrentIndex(0);

    // Cue recall on.
    checkBoxSeekToCue->setChecked(true);

    // Don't start in full screen.
    checkBoxStartFullScreen->setChecked(false);

    // Tooltips on.
    checkBoxTooltipsEnabled->setChecked(true);
    checkBoxTooltipsOnlyLibrary->setChecked(false);

    // Rate-ramping default off.
    groupBoxRateRamp->setChecked(false);

    // 0 rate-ramp sensitivity
    SliderRateRampSensitivity->setValue(0);

    // Permanent and temporary pitch adjust fine/coarse.
    spinBoxTempRateLeft->setValue(4.0);
    spinBoxTempRateRight->setValue(2.0);
    spinBoxPermRateLeft->setValue(0.50);
    spinBoxPermRateRight->setValue(0.05);

    // Don't automatically reset the pitch & speed on track load
    m_speedAutoReset = 0;
    checkBoxResetSpeedAndPitch->setChecked(false);

    // Lock to original key
    m_keylockMode = 0;
    radioButtonOriginalKey->setChecked(true);
}

void DlgPrefControls::slotSetLocale(int pos) {
    QString newLocale = ComboBoxLocale->itemData(pos).toString();
    m_pConfig->set(ConfigKey("[Config]", "Locale"), ConfigValue(newLocale));
    notifyRebootNecessary();
}

void DlgPrefControls::slotSetRateRange(int pos) {
    double range = static_cast<double>(pos-1) / 10.0;
    if (pos == 0)
        range = 0.06;
    if (pos == 1)
        range = 0.08;

    qDebug() << "slotSetRateRange" << pos << range;

    // Set rate range for every group
    foreach (ControlObjectThread* pControl, m_rateRangeControls) {
        pControl->slotSet(range);
    }

    // Reset rate for every group
    foreach (ControlObjectThread* pControl, m_rateControls) {
        pControl->slotSet(0);
    }
}

void DlgPrefControls::slotSetRateDir(bool invert) {
    int index = 0;
    if (invert) index = 1;
    slotSetRateDir(index);
}

void DlgPrefControls::slotSetRateDir(int index) {
    float dir = 1.;
    if (index == 1)
        dir = -1.;
    float oldDir = m_rateDirControls[0]->get();

    // Set rate direction for every group
    foreach (ControlObjectThread* pControl, m_rateDirControls) {
        pControl->slotSet(dir);
    }

    // If the setting was changed, ie the old direction is not equal to the new one,
    // multiply the rate by -1 so the current sound does not change.
    if(fabs(dir - oldDir) > 0.1) {
        foreach (ControlObjectThread* pControl, m_rateControls) {
            pControl->slotSet(-1 * pControl->get());
        }
    }

}

void DlgPrefControls::slotKeyLockMode(QAbstractButton* b) {
    if (b == radioButtonCurrentKey) {
        m_keylockMode = 1;
    }
    else { m_keylockMode = 0; }
}

void DlgPrefControls::slotSetAllowTrackLoadToPlayingDeck(bool b) {
    // If b is true, it means NOT to allow track loading
    m_pConfig->set(ConfigKey("[Controls]", "AllowTrackLoadToPlayingDeck"),
                   ConfigValue(b?0:1));
}

void DlgPrefControls::slotSetCueDefault(int)
{
    int cueIndex = ComboBoxCueDefault->currentIndex();
    m_pConfig->set(ConfigKey("[Controls]", "CueDefault"), ConfigValue(cueIndex));

    // Set cue behavior for every group
    foreach (ControlObjectThread* pControl, m_cueControls) {
        pControl->slotSet(cueIndex);
    }
}

void DlgPrefControls::slotSetCueRecall(bool b)
{
    m_pConfig->set(ConfigKey("[Controls]", "CueRecall"), ConfigValue(b?0:1));
}

void DlgPrefControls::slotSetStartInFullScreen(bool b) {
    m_pConfig->set(ConfigKey("[Config]", "StartInFullscreen"), ConfigValue(b?1:0));
}

void DlgPrefControls::slotSetTooltips() {
    //0=OFF, 1=ON, 2=ON (only in Library)
    int valueToSet = 1;
    if (checkBoxTooltipsEnabled->isChecked()) {
        checkBoxTooltipsOnlyLibrary->setDisabled(false);
        if (checkBoxTooltipsOnlyLibrary->isChecked()) {
            valueToSet = 2;
        }
    }
    else {
        checkBoxTooltipsOnlyLibrary->setDisabled(true);
        valueToSet = 0;
    }
    m_mixxx->setToolTipsCfg(valueToSet);
}

void DlgPrefControls::notifyRebootNecessary() {
    // make the fact that you have to restart mixxx more obvious
    QMessageBox::information(
        this, tr("Information"),
        tr("Mixxx must be restarted before the changes will take effect."));
}

void DlgPrefControls::slotSetScheme(int) {
    m_pConfig->set(ConfigKey("[Config]", "Scheme"), ComboBoxSchemeconf->currentText());
    m_mixxx->rebootMixxxView();
}

void DlgPrefControls::slotSetSkin(int) {
    m_pConfig->set(ConfigKey("[Config]", "ResizableSkin"), ComboBoxSkinconf->currentText());
    m_mixxx->rebootMixxxView();
    checkSkinResolution(ComboBoxSkinconf->currentText())
            ? warningLabel->hide() : warningLabel->show();
    slotUpdateSchemes();
}

void DlgPrefControls::slotSetTrackTimeDisplay(QAbstractButton* b) {
    int timeDisplay = 0;
    if (b == radioButtonRemaining) {
        timeDisplay = 1;
        m_pConfig->set(ConfigKey("[Controls]","PositionDisplay"), ConfigValue(1));
    }
    else {
        m_pConfig->set(ConfigKey("[Controls]","PositionDisplay"), ConfigValue(0));
    }
    m_pControlTrackTimeDisplay->set(timeDisplay);
}

void DlgPrefControls::slotSetTrackTimeDisplay(double v) {
    if (v > 0) {
        // Remaining
        radioButtonRemaining->setChecked(true);
        m_pConfig->set(ConfigKey("[Controls]", "PositionDisplay"), ConfigValue(1));
    } else {
        // Elapsed
        radioButtonElapsed->setChecked(true);
        m_pConfig->set(ConfigKey("[Controls]", "PositionDisplay"), ConfigValue(0));
    }
}

void DlgPrefControls::slotSetRateTempLeft(double v) {
    QString str;
    str = str.setNum(v, 'f');
    m_pConfig->set(ConfigKey("[Controls]", "RateTempLeft"),ConfigValue(str));
    RateControl::setTemp(v);
}

void DlgPrefControls::slotSetRateTempRight(double v) {
    QString str;
    str = str.setNum(v, 'f');
    m_pConfig->set(ConfigKey("[Controls]", "RateTempRight"),ConfigValue(str));
    RateControl::setTempSmall(v);
}

void DlgPrefControls::slotSetRatePermLeft(double v) {
    QString str;
    str = str.setNum(v, 'f');
    m_pConfig->set(ConfigKey("[Controls]", "RatePermLeft"),ConfigValue(str));
    RateControl::setPerm(v);
}

void DlgPrefControls::slotSetRatePermRight(double v) {
    QString str;
    str = str.setNum(v, 'f');
    m_pConfig->set(ConfigKey("[Controls]", "RatePermRight"),ConfigValue(str));
    RateControl::setPermSmall(v);
}

void DlgPrefControls::slotSetRateRampSensitivity(int sense) {
    m_pConfig->set(ConfigKey("[Controls]", "RateRampSensitivity"),
                   ConfigValue(SliderRateRampSensitivity->value()));
    RateControl::setRateRampSensitivity(sense);
}

void DlgPrefControls::slotSetRateRamp(bool mode) {
    m_pConfig->set(ConfigKey("[Controls]", "RateRamp"),
                   ConfigValue(groupBoxRateRamp->isChecked()));
    RateControl::setRateRamp(mode);
}

void DlgPrefControls::slotApply() {
    double deck1RateRange = m_rateRangeControls[0]->get();
    double deck1RateDir = m_rateDirControls[0]->get();

    // Write rate range to config file
    double idx = (10. * deck1RateRange) + 1;
    if (deck1RateRange <= 0.07)
        idx = 0.;
    else if (deck1RateRange <= 0.09)
        idx = 1.;

    m_pConfig->set(ConfigKey("[Controls]", "RateRange"), ConfigValue((int)idx));

    // Write rate direction to config file
    if (deck1RateDir == 1) {
        m_pConfig->set(ConfigKey("[Controls]", "RateDir"), ConfigValue(0));
    } else {
        m_pConfig->set(ConfigKey("[Controls]", "RateDir"), ConfigValue(1));
    }

    m_pConfig->set(ConfigKey("[Controls]", "SpeedAutoReset"),
            ConfigValue(m_speedAutoReset));

    m_pConfig->set(ConfigKey("[Controls]", "PitchAndKeylockMode"),
            ConfigValue(m_keylockMode));
    // Set key lock behavior for every group
    foreach (ControlObjectThread* pControl, m_keylockModeControls) {
        pControl->slotSet(m_keylockMode);
    }
}

//Returns TRUE if skin fits to screen resolution, FALSE otherwise
bool DlgPrefControls::checkSkinResolution(QString skin)
{
    int screenWidth = QApplication::desktop()->width();
    int screenHeight = QApplication::desktop()->height();

    const QRegExp min_size_regex("<MinimumSize>(\\d+), *(\\d+)<");
    QFile skinfile(skin + "/skin.xml");
    if (skinfile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&skinfile);
        bool found_size = false;
        while (!in.atEnd()) {
            if (min_size_regex.indexIn(in.readLine()) != -1) {
                found_size = true;
                break;
            }
        }
        if (found_size) {
            return !(min_size_regex.cap(1).toInt() > screenWidth ||
                     min_size_regex.cap(2).toInt() > screenHeight);
        }
    }

    // If regex failed, fall back to skin name parsing.
    QString skinName = skin.left(skin.indexOf(QRegExp("\\d")));
    QString resName = skin.right(skin.count()-skinName.count());
    QString res = resName.left(resName.lastIndexOf(QRegExp("\\d"))+1);
    QString skinWidth = res.left(res.indexOf("x"));
    QString skinHeight = res.right(res.count()-skinWidth.count()-1);
    return !(skinWidth.toInt() > screenWidth || skinHeight.toInt() > screenHeight);
}

void DlgPrefControls::slotNumDecksChanged(double new_count) {
    int numdecks = static_cast<int>(new_count);
    if (numdecks <= m_iNumConfiguredDecks) {
        // TODO(owilliams): If we implement deck deletion, shrink the size of configured decks.
        return;
    }

    for (int i = m_iNumConfiguredDecks; i < numdecks; ++i) {
        QString group = PlayerManager::groupForDeck(i);
        m_rateControls.push_back(new ControlObjectThread(
                group, "rate"));
        m_rateRangeControls.push_back(new ControlObjectThread(
                group, "rateRange"));
        m_rateDirControls.push_back(new ControlObjectThread(
                group, "rate_dir"));
        m_cueControls.push_back(new ControlObjectThread(
                group, "cue_mode"));
        m_keylockModeControls.push_back(new ControlObjectThread(
                        group, "keylockMode"));
        m_keylockModeControls.last()->set(m_keylockMode);
    }

    m_iNumConfiguredDecks = numdecks;
    slotSetRateDir(m_pConfig->getValueString(ConfigKey("[Controls]", "RateDir")).toInt());
    slotSetRateRange(m_pConfig->getValueString(ConfigKey("[Controls]", "RateRange")).toInt());
}

void DlgPrefControls::slotNumSamplersChanged(double new_count) {
    int numsamplers = static_cast<int>(new_count);
    if (numsamplers <= m_iNumConfiguredSamplers) {
        return;
    }

    for (int i = m_iNumConfiguredSamplers; i < numsamplers; ++i) {
        QString group = PlayerManager::groupForSampler(i);
        m_rateControls.push_back(new ControlObjectThread(
                group, "rate"));
        m_rateRangeControls.push_back(new ControlObjectThread(
                group, "rateRange"));
        m_rateDirControls.push_back(new ControlObjectThread(
                group, "rate_dir"));
        m_cueControls.push_back(new ControlObjectThread(
                group, "cue_mode"));
        m_keylockModeControls.push_back(new ControlObjectThread(
                        group, "keylockMode"));
        m_keylockModeControls.last()->set(m_keylockMode);
    }

    m_iNumConfiguredSamplers = numsamplers;
    slotSetRateDir(m_pConfig->getValueString(ConfigKey("[Controls]", "RateDir")).toInt());
    slotSetRateRange(m_pConfig->getValueString(ConfigKey("[Controls]", "RateRange")).toInt());
}

void DlgPrefControls::slotUpdateSpeedAutoReset(bool b) {
    if (b) m_speedAutoReset = 1;
    else m_speedAutoReset = 0;
}
