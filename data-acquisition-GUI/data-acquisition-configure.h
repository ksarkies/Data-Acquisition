/*          Data Acquisition GUI Configure Window Header

@date 2 January 2017
*/

/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies                                      *
 *   ksarkies@internode.on.net                                              *
 *                                                                          *
 *   This file is part of Data Acquisition GUI                              *
 *                                                                          *
 *   Data Acquisition GUI is free software; you can redistribute it and/or  *
 *   modify it under the terms of the GNU General Public License as         *
 *   published by the Free Software Foundation; either version 2 of the     *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   Data Acquisition GUI is distributed in the hope that it will be useful,*
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with Data Acquisition GUI if not, write to the                   *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#ifndef DATA_ACQUISITION_CONFIGURE_H
#define DATA_ACQUISITION_CONFIGURE_H

#include "data-acquisition.h"
//#include "ui_data-acquisition-configure.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDialog>

//-----------------------------------------------------------------------------
/** @brief Data Acquisition Configure Window.

*/
/*
class DataAcquisitionConfigGui : public QDialog
{
    Q_OBJECT
public:
    DataAcquisitionConfigGui(SerialPort*, QWidget* parent = 0);
    ~DataAcquisitionConfigGui();
    QString error();
private slots:
    void on_closeButton_clicked();
    void on_timeSetButton_clicked();
    void on_debugMessageCheckbox_clicked();
    void on_dataMessageCheckbox_clicked();
    void on_echoTestButton_clicked();
    void on_queryBatteryButton_clicked();
    void on_setBatteryButton_clicked();
    void on_battery1TypeCombo_activated(int index);
    void on_battery1CapacitySpinBox_valueChanged(int i);
    void on_battery2TypeCombo_activated(int index);
    void on_battery2CapacitySpinBox_valueChanged(int i);
    void on_battery3TypeCombo_activated(int index);
    void on_battery3CapacitySpinBox_valueChanged(int i);
    void on_resetMissing1Button_clicked();
    void on_resetMissing2Button_clicked();
    void on_resetMissing3Button_clicked();
    void on_forceZeroCurrent1_clicked();
    void on_forceZeroCurrent2_clicked();
    void on_forceZeroCurrent3_clicked();
    void on_calibrateButton_clicked();
    void on_setTrackOptionButton_clicked();
    void on_setChargeOptionButton_clicked();
    void on_absorptionMuteCheckbox_clicked();
    void onMessageReceived(const QString &text);
    void displayErrorMessage(const QString message);
private:
// User Interface object instance
    Ui::DataAcquisitionConfigDialog DataAcquisitionConfigUi;
    QSerialPort *socket;           //!< Serial port object pointer
    QString errorMessage;
    QString response;           // String to build a line of characters
    QString quiescentCurrent;
};
*/
#endif
