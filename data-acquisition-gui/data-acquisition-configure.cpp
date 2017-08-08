/*       Data Acquisition Configure Window

This window provides overall configuration.

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
#include "data-acquisition-configure.h"
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

DataAcquisitionConfigGui::DataAcquisitionConfigGui(QSerialPort* p, QWidget* parent)
                                                    : QDialog(parent)
{
    socket = p;
    DataAcquisitionConfigUi.setupUi(this);
/* Ask for identification */
    socket->write("dE\n\r");
}

DataAcquisitionConfigGui::~DataAcquisitionConfigGui()
{
}

//-----------------------------------------------------------------------------
/** @brief Set the Time

The current time is read and transmitted to set time in the remote system.
*/

void DataAcquisitionConfigGui::on_timeSetButton_clicked()
{
    QDateTime localDateTime = QDateTime::currentDateTime();
    localDateTime.setTimeSpec(Qt::UTC);
    socket->write("pH");
    socket->write(localDateTime.toString(Qt::ISODate).append("\n\r")
                               .toLatin1().constData());
}

//-----------------------------------------------------------------------------
/** @brief Enable Debug Messages

*/

void DataAcquisitionConfigGui::on_debugMessageCheckbox_clicked()
{
    if (DataAcquisitionConfigUi.debugMessageCheckbox->isChecked())
        socket->write("pd+\n\r");
    else
        socket->write("pd-\n\r");
}

//-----------------------------------------------------------------------------
/** @brief Enable Data Messages

*/

void DataAcquisitionConfigGui::on_dataMessageCheckbox_clicked()
{
    if (DataAcquisitionConfigUi.dataMessageCheckbox->isChecked())
        socket->write("pM+\n\r");
    else
        socket->write("pM-\n\r");
}

//-----------------------------------------------------------------------------
/** @brief Send Echo Request

*/

void DataAcquisitionConfigGui::on_echoTestButton_clicked()
{
    socket->write("aE\n\r");
    socket->write("dS\n\r");
}

//-----------------------------------------------------------------------------
/** @brief Close Window

*/

void DataAcquisitionConfigGui::on_closeButton_clicked()
{
    this->close();
}

//-----------------------------------------------------------------------------
/** @brief Process a Message.

After a command is sent, response messages from the remote are passed here for
processing. Appropriate fields on the form are updated.

The message type can be p (parameter) or one of three cases d (data message).
The function doesn't check the type, only the command.

@todo This interprets response as done in main. See if this can be made
independent of formats.
*/

void DataAcquisitionConfigGui::onMessageReceived(const QString &response)
{
    QStringList breakdown = response.split(",");
    int size = breakdown.size();
    if (size <= 0) return;
    QChar command = breakdown[0].at(1);
// Check for ident response
    if (command == 'E')
    {
        if (size != 3) return;
        DataAcquisitionConfigUi.firmwareVersion->setText("Firmware version : " + breakdown[2]);
        return;
    }
// Error Code
    switch (command.toLatin1())
    {
// Show current time settings from the system
        case 'H':
        {
            if (size < 2) break;
            QDateTime systemTime =
                QDateTime::fromString(breakdown[1].simplified(),Qt::ISODate);
            DataAcquisitionConfigUi.date->setText(systemTime.date().toString("dd.MM.yyyy"));
            DataAcquisitionConfigUi.time->setText(systemTime.time().toString("H.mm.ss"));
            break;
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Error Message

@returns a message when the device didn't respond.
*/

QString DataAcquisitionConfigGui::error()
{
    return errorMessage;
}

//-----------------------------------------------------------------------------
/** @brief Show an error condition in the Error label.

@param[in] message Message to be displayed.
*/

void DataAcquisitionConfigGui::displayErrorMessage(const QString message)
{
    DataAcquisitionConfigUi.errorLabel->setText(message);
}

