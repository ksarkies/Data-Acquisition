/*          Data Acquisition GUI Recording Window Header

@date 6 August 2016
*/

/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies                                      *
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

#ifndef DATA_ACQUISITION_RECORD_H
#define DATA_ACQUISITIONN_RECORD_H

#include "data-acquisition.h"
#include "ui_data-acquisition-record.h"
#include <QDialog>
#include <QStandardItemModel>

//-----------------------------------------------------------------------------
/** @brief Data Acquisition Recording Window.

*/

class DataAcquisitionRecordGui : public QDialog
{
    Q_OBJECT
public:
#ifdef SERIAL
    DataAcquisitionRecordGui(SerialPort* socket, QWidget* parent = 0);
#else
    DataAcquisitionRecordGui(QTcpSocket* socket, QWidget* parent = 0);
#endif
    ~DataAcquisitionRecordGui();
private slots:
    void on_deleteButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_closeFileButton_clicked();
    void on_recordFileButton_clicked();
    void onMessageReceived(const QString &text);
    void onListItemClicked(const QModelIndex & index);
    void on_registerButton_clicked();
    void on_closeButton_clicked();
private:
// User Interface object instance
    Ui::DataAcquisitionRecordDialog WeatherStationRecordUi;
    int extractValue(const QString &response);
    void requestRecordingStatus();
    void refreshDirectory();
    void getFreeSpace();
    int writeFileHandle;
    int readFileHandle;
    bool recordingOn;
    bool writeFileOpen;
    bool readFileOpen;
    QStandardItemModel *model;
    int row;
    bool directoryEnded;
    bool nextDirectoryEntry;

};

#endif
