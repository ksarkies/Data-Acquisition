/*       Data Acquisition Test Window

This window provides configuration and controls for a number of tests.

@date 4 August 2017
*/
/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies                                      *
 *   ksarkies@internode.on.net                                              *
 *                                                                          *
 *   This file is part of data-acquisition                                  *
 *                                                                          *
 *   data-acquisition is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU General Public License as         *
 *   published by the Free Software Foundation; either version 2 of the     *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   data-acquisition is distributed in the hope that it will be useful,    *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with data-acquisition if not, write to the                       *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#include "data-acquisition-main.h"
#include "data-acquisition-test.h"
#include <QApplication>
#include <QString>
#include <QLabel>
#include <QSpinBox>
#include <QCloseEvent>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

//-----------------------------------------------------------------------------
/** Test Window GUI Constructor

@param[in] socket TCP Socket object pointer
@param[in] parent Parent widget.
*/

DataAcquisitionTestGui::DataAcquisitionTestGui(QSerialPort* p, QWidget* parent)
                                                    : QDialog(parent)
{
    socket = p;
    DataAcquisitionTestUi.setupUi(this);
    on_manualButton_clicked();
}

DataAcquisitionTestGui::~DataAcquisitionTestGui()
{
}

//-----------------------------------------------------------------------------
/** @brief Manual Test Run.

*/

void DataAcquisitionTestGui::on_manualButton_clicked()
{
    DataAcquisitionTestUi.durationSpinBox->setVisible(false);
    DataAcquisitionTestUi.durationLabel->setVisible(false);
    DataAcquisitionTestUi.voltageSpinBox->setVisible(true);
    DataAcquisitionTestUi.voltageLabel->setText("Minimum Voltage");
    DataAcquisitionTestUi.voltageLabel->setVisible(true);
}

//-----------------------------------------------------------------------------
/** @brief Timed Test Run.

*/

void DataAcquisitionTestGui::on_timerButton_clicked()
{
    DataAcquisitionTestUi.durationSpinBox->setVisible(true);
    DataAcquisitionTestUi.durationLabel->setText("Run Time (minutes)");
    DataAcquisitionTestUi.durationLabel->setVisible(true);
    DataAcquisitionTestUi.voltageSpinBox->setVisible(true);
    DataAcquisitionTestUi.voltageLabel->setText("Minimum Voltage");
    DataAcquisitionTestUi.voltageLabel->setVisible(true);
}

//-----------------------------------------------------------------------------
/** @brief Voltage Level Test Run.

*/

void DataAcquisitionTestGui::on_voltageButton_clicked()
{
    DataAcquisitionTestUi.durationSpinBox->setVisible(false);
    DataAcquisitionTestUi.durationLabel->setVisible(false);
    DataAcquisitionTestUi.voltageSpinBox->setVisible(true);
    DataAcquisitionTestUi.voltageLabel->setText("Voltage Level");
    DataAcquisitionTestUi.voltageLabel->setVisible(true);
}

//-----------------------------------------------------------------------------
/** @brief Start Test.

Connect loads and sources to devices under test according to the settings in
the checkboxes.
*/

void DataAcquisitionTestGui::on_startButton_clicked()
{
    if (DataAcquisitionTestUi.loadCheckBox_1->isChecked())
    {
        if (DataAcquisitionTestUi.deviceCheckBox_1->isChecked())
            socket->write("aS11\n\r");
        if (DataAcquisitionTestUi.deviceCheckBox_2->isChecked())
            socket->write("aS21\n\r");
        if (DataAcquisitionTestUi.deviceCheckBox_3->isChecked())
            socket->write("aS31\n\r");
    }
    if (DataAcquisitionTestUi.loadCheckBox_2->isChecked())
    {
        if (DataAcquisitionTestUi.deviceCheckBox_1->isChecked())
            socket->write("aS12\n\r");
        if (DataAcquisitionTestUi.deviceCheckBox_2->isChecked())
            socket->write("aS22\n\r");
        if (DataAcquisitionTestUi.deviceCheckBox_3->isChecked())
            socket->write("aS32\n\r");
    }
    if (DataAcquisitionTestUi.sourceCheckBox->isChecked())
    {
        if (DataAcquisitionTestUi.deviceCheckBox_1->isChecked())
            socket->write("aS13\n\r");
        if (DataAcquisitionTestUi.deviceCheckBox_2->isChecked())
            socket->write("aS23\n\r");
        if (DataAcquisitionTestUi.deviceCheckBox_3->isChecked())
            socket->write("aS33\n\r");
    }
}

//-----------------------------------------------------------------------------
/** @brief Stop Test.

Disconnect all loads and sources.
*/

void DataAcquisitionTestGui::on_stopButton_clicked()
{
    socket->write("aS01\n\r");
    socket->write("aS02\n\r");
    socket->write("aS03\n\r");
}

//-----------------------------------------------------------------------------
/** @brief Close Window

*/

void DataAcquisitionTestGui::on_closeButton_clicked()
{
    this->close();
}


