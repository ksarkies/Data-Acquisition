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
}

DataAcquisitionConfigGui::~DataAcquisitionConfigGui()
{
}

//-----------------------------------------------------------------------------
/** @brief Start Test.

*/

void DataAcquisitionConfigGui::on_startButton_clicked()
{
}

//-----------------------------------------------------------------------------
/** @brief Stop Test.

*/

void DataAcquisitionConfigGui::on_stopButton_clicked()
{
}

//-----------------------------------------------------------------------------
/** @brief Close Window

*/

void DataAcquisitionConfigGui::on_closeButton_clicked()
{
    this->close();
}


