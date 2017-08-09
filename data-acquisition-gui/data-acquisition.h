/*       Data Acquisition Main Window

Here the data stream from the remote is received and saved to a file.

@date 31 December 2016
*/
/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies                                      *
 *   ksarkies@internode.on.net                                              *
 *                                                                          *
 *   This file is part of Data Acquisition Project                          *
 *                                                                          *
 *   Data Acquisition is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU General Public License as         *
 *   published by the Free Software Foundation; either version 2 of the     *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   Data Acquisition is distributed in the hope that it will be useful,    *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with Data Acquisition if not, write to the                       *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#ifndef DATA_ACQUISITIONN_H
#define DATA_ACQUISITIONN_H

#include "ui_data-acquisition-main.h"
#include "data-acquisition-main.h"
#include <QDir>
#include <QFile>
#include <QTime>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QListWidgetItem>
#include <QDialog>
#include <QCloseEvent>

#define millisleep(a) usleep(a*1000)

//-----------------------------------------------------------------------------
/** @brief Data Acquisition Main Window.

*/

class DataAcquisitionGui : public QDialog
{
    Q_OBJECT
public:
    DataAcquisitionGui(QString device, uint parameter, QWidget* parent = 0);
    ~DataAcquisitionGui();
    bool success();
    QString error();
private slots:
    void on_connectButton_clicked();
    void onDataAvailable();
    void on_saveFileButton_clicked();
    void on_closeFileButton_clicked();
    void on_recordingButton_clicked();
    void on_configureButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_manualButton_clicked();
    void on_timerButton_clicked();
    void on_voltageButton_clicked();
    void closeEvent(QCloseEvent*);
signals:
    void recordMessageReceived(const QString response);
    void configureMessageReceived(const QString response);
private:
// User Interface object instance
    Ui::DataAcquisitionMainDialog DataAcquisitionMainUi;
// Common code
    void initMainWindow(Ui::DataAcquisitionMainDialog);
    void setSourceComboBox(int index);
// Methods
    void processResponse(const QString response);
    void displayErrorMessage(const QString message);
    void saveLine(QString line);    // Save line to a file
    void ssleep(int seconds);
    int activeInterfaces(void);
// Variables
    QString serialDevice;
    uint baudrate;
    bool synchronized;
    QString errorMessage;
    QString response;
    QSerialPort* port;           	//!< Serial port object pointer
    QDir saveDirectory;
    QString saveFile;
    QFile* outFile;
    QTime tick;
    long testTime;
    long timeElapsed;
    int voltageLimit;
    int testType;
    bool testRunning;
};

#endif
