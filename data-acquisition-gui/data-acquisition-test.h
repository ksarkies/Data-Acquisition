/*          Data Acquisition GUI Test Window Header

@date 4 August 2017
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
 *   Data Acquisition GUI is distributed in the hope that it will be useful *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with Data Acquisition GUI if not, write to the                   *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#ifndef DATA_ACQUISITION_TEST_H
#define DATA_ACQUISITION_TEST_H

#include "data-acquisition.h"
#include "ui_data-acquisition-test.h"
#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

//-----------------------------------------------------------------------------
/** @brief Data Acquisition Test Window.

*/

class DataAcquisitionTestGui : public QDialog
{
    Q_OBJECT
public:
    DataAcquisitionTestGui(QSerialPort* socket, int setting, QWidget* parent = 0);
    ~DataAcquisitionTestGui();
private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_closeButton_clicked();
    void on_manualButton_clicked();
    void on_timerButton_clicked();
    void on_voltageButton_clicked();
private:
// User Interface object instance
    int interfaces;
    Ui::DataAcquisitionTestDialog DataAcquisitionTestUi;
    QSerialPort* socket;
};

#endif
