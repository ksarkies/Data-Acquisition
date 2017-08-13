/**
@mainpage Power Management Data Processing Main Window
@version 1.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 08 August 2017

Utility program to aid in analysis if BMS data files.
*/

/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies                                      *
 *   ksarkies@internode.on.net                                              *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU General Public License as         *
 *   published by the Free Software Foundation; either version 2 of the     *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program if not, write to the                           *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/


#include "data-processing-main.h"
#include <QApplication>
#include <QString>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QTextEdit>
#include <QCloseEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_date_scale_draw.h>
#include <qwt_date_scale_engine.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

//-----------------------------------------------------------------------------
/** Power Management Data Processing Main Window Constructor

@param[in] parent Parent widget.
*/

DataProcessingGui::DataProcessingGui()
{
// Build the User Interface display from the Ui class in ui_mainwindowform.h
    DataProcessingMainUi.setupUi(this);
    outFile = NULL;
}

DataProcessingGui::~DataProcessingGui()
{
}

//-----------------------------------------------------------------------------
/** @brief Successful establishment of Window

@returns TRUE if successful.
*/
bool DataProcessingGui::success()
{
    return true;
}

//-----------------------------------------------------------------------------
/** @brief Open a raw data file for Reading.

This button only opens the file for reading.
*/

void DataProcessingGui::on_openReadFileButton_clicked()
{
    QString errorMessage;
    QFileInfo fileInfo;
    QString filename = QFileDialog::getOpenFileName(this,
                                "Data File","./","Text Files (*.txt)");
    if (filename.isEmpty())
    {
        displayErrorMessage("No filename specified");
        return;
    }
    inFile = new QFile(filename);
    fileInfo.setFile(filename);
/* Look for start and end times, and determine current zero calibration */
    if (inFile->open(QIODevice::ReadOnly))
    {
        scanFile(inFile);
    }
    else
    {
        displayErrorMessage(QString("%1").arg((uint)inFile->error()));
    }
}

//-----------------------------------------------------------------------------
/** @brief Extract All to CSV.

The data files are expected to have the format of the BMS transmissions and
will be text files. 

All data is extracted to a csv file with one record per time interval. The
records subsequent to the time record are analysed and data is extracted
to the appropriate fields. The code expects the records to have a particular
order as sent by the BMS and the output has the same order without any
identification. The output format is suitable for spreadsheet analysis.
*/

void DataProcessingGui::on_dumpAllButton_clicked()
{
    QDateTime startTime = DataProcessingMainUi.startTime->dateTime();
    QDateTime endTime = DataProcessingMainUi.endTime->dateTime();
    if (! inFile->isOpen()) return;
    if (! openSaveFile()) return;
    inFile->seek(0);      // rewind input file
    combineRecords(startTime, endTime, inFile, outFile, true);
    if (saveFile.isEmpty())
        displayErrorMessage("File already closed");
    else
    {
        outFile->close();
        delete outFile;
//! Clear the name to prevent the same file being used.
        saveFile = QString();
    }
}

//-----------------------------------------------------------------------------
/** @brief Select Voltages to be plotted
*/

void DataProcessingGui::on_voltagePlotCheckBox_clicked()
{
    DataProcessingMainUi.temperaturePlotCheckbox->setChecked(false);
}

//-----------------------------------------------------------------------------
/** @brief Action taken on temperature checkbox selected
*/

void DataProcessingGui::on_temperaturePlotCheckbox_clicked()
{
    if (DataProcessingMainUi.temperaturePlotCheckbox->isChecked())
    {
        DataProcessingMainUi.device1Checkbox->setChecked(false);
        DataProcessingMainUi.device2Checkbox->setChecked(false);
        DataProcessingMainUi.device3Checkbox->setChecked(false);
        DataProcessingMainUi.voltagePlotCheckBox->setChecked(false);
    }
}

//-----------------------------------------------------------------------------
/** @brief Select File to be plotted and execute the plot

@todo This procedure deals with all valid plots and as such is a bit involved.
Later split out into a separate window with different procedures and more
options.
*/

void DataProcessingGui::on_plotFileSelectButton_clicked()
{
    bool showCurrent = ! DataProcessingMainUi.voltagePlotCheckBox->isChecked();
    bool showTemperature = DataProcessingMainUi.temperaturePlotCheckbox->isChecked();
    float yScaleLow,yScaleHigh;
    bool showPlot1,showPlot2,showPlot3;
    int i1 = 0;                // Columns for data series
    int i2 = 0;
    int i3 = 0;

// Get data file
    QString fileName = QFileDialog::getOpenFileName(0,
                                "Data File","./","CSV Files (*.csv)");
    if (fileName.isEmpty()) return;
    QFileInfo fileInfo;
    QFile* inFile = new QFile(fileName);
    fileInfo.setFile(fileName);
    if (! inFile->open(QIODevice::ReadOnly)) return;
    QTextStream inStream(inFile);

// Setup Plot objects
    QwtPlotCurve *curve1;
    curve1 = new QwtPlotCurve();
    QPolygonF points1;
    QwtPlotCurve *curve2;
    curve2 = new QwtPlotCurve();
    QPolygonF points2;
    QwtPlotCurve *curve3;
    curve3 = new QwtPlotCurve();
    QPolygonF points3;

// At present Temperature ticked shows only the one plot.
    if (showTemperature)       // Temperature
    {
        showPlot1 = true;
        showPlot2 = false;
        showPlot3 = false;
        i1 = 25;
        yScaleLow = -10;
        yScaleHigh = 50;
    }
    else if (showCurrent)           // Current
    {
        showPlot1 = DataProcessingMainUi.device1Checkbox->isChecked();
        showPlot2 = DataProcessingMainUi.device2Checkbox->isChecked();
        showPlot3 = DataProcessingMainUi.device3Checkbox->isChecked();
        i1 = 1;
        i2 = 3;
        i3 = 5;
        yScaleLow = -20;
        yScaleHigh = 20;
    }
    else                            // Voltage
    {
        showPlot1 = DataProcessingMainUi.device1Checkbox->isChecked();
        showPlot2 = DataProcessingMainUi.device2Checkbox->isChecked();
        showPlot3 = DataProcessingMainUi.device3Checkbox->isChecked();
        i1 = 2;
        i2 = 4;
        i3 = 6;
        yScaleLow = 10;
        yScaleHigh = 18;
    }

// Set display parameters and titles
    if (showPlot1)
    {
        if (showTemperature) curve1->setTitle("Temperature");
        else curve1->setTitle("device 1");
        curve1->setPen(Qt::blue, 2),
        curve1->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    }
    if (showPlot2)
    {
        curve2->setTitle("device 2");
        curve2->setPen(Qt::red, 2),
        curve2->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    }
    if (showPlot3)
    {
        curve3->setTitle("device 3");
        curve3->setPen(Qt::yellow, 2),
        curve3->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    }

    bool ok;
// Read in data from input file
// Skip first line as it may be a header
  	QString lineIn;
    lineIn = inStream.readLine();
    bool startRun = true;
// Index increments by about 0.5 seconds
// To have x-axis in date-time index must be "double" type, ie ms since epoch.
    double index = 0;
    QDateTime startTime;
    QDateTime previousTime;
    while (! inStream.atEnd())
    {
      	lineIn = inStream.readLine();
        QStringList breakdown = lineIn.split(",");
        int size = breakdown.size();
        if (size == LINE_WIDTH)
        {
            QDateTime time = QDateTime::fromString(breakdown[0].simplified(),Qt::ISODate);
            if (time.isValid())
            {
// On the first run get the start time
                if (startRun)
                {
                    startRun = false;
                    startTime = time;
                    previousTime = startTime;
                    index = startTime.toMSecsSinceEpoch();
                }
// Try to keep index and time in sync to account for jumps in time.
// Index is counting half seconds and time from records is integer seconds only
                if (previousTime == time) index += 500;
                else index = time.toMSecsSinceEpoch();
// Create points to plot
                if (showPlot1)
                {
                    float currentdevice1 = breakdown[i1].simplified().toFloat(&ok);
                    points1 << QPointF(index,currentdevice1);
                }
                if (showPlot2)
                {
                    float currentdevice2 = breakdown[i2].simplified().toFloat(&ok);
                    points2 << QPointF(index,currentdevice2);
                }
                if (showPlot3)
                {
                    float currentdevice3 = breakdown[i3].simplified().toFloat(&ok);
                    points3 << QPointF(index,currentdevice3);
                }
            }
        }
    }
// Build plot
    QwtPlot *plot = new QwtPlot(0);
    if (showTemperature) plot->setTitle("device Temperature");
    else
    {
        if (showCurrent) plot->setTitle("device Currents");
        else plot->setTitle("device Voltages");
    }
    plot->setCanvasBackground(Qt::white);
    plot->setAxisScale(QwtPlot::yLeft, yScaleLow, yScaleHigh);
    //Set x-axis scaling.
    QwtDateScaleDraw *qwtDateScaleDraw = new QwtDateScaleDraw(Qt::LocalTime);
    QwtDateScaleEngine *qwtDateScaleEngine = new QwtDateScaleEngine(Qt::LocalTime);
    qwtDateScaleDraw->setDateFormat(QwtDate::Hour, "hh");
 
    plot->setAxisScaleDraw(QwtPlot::xBottom, qwtDateScaleDraw);
    plot->setAxisScaleEngine(QwtPlot::xBottom, qwtDateScaleEngine);
//    plot->setAxisScale(QwtPlot::xBottom, xScaleLow, xScaleHigh);
    plot->insertLegend(new QwtLegend());
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->attach(plot);

    if (showPlot1)
    {
        curve1->setSamples(points1);
        curve1->attach(plot);
    }
    if (showPlot2)
    {
        curve2->setSamples(points2);
        curve2->attach(plot);
    }
    if (showPlot3)
    {
        curve3->setSamples(points3);
        curve3->attach(plot);
    }

    plot->resize(1000,600);
    plot->show();
}

//-----------------------------------------------------------------------------
/** @brief Analysis of CSV files for various performance indicators.

The file is analysed for a variety of faults and other performance indicators.

The results are printed out to a report file.

- Situations where the charger is not allocated but a device is ready. To show
  this look for no device under charge and source voltage above any device.
  Print the op state, charging phase, device and source voltages, switches,
  decision and indicators.

- device current during the charging phase to show in particular how the float
  state is reached and if this is a true indication of a device being fully
  charged.

- Solar current input derived from all batteries as they are charged in the bulk
  phase. This may cut short during any one day if all batteries enter the float
  state and the charger is de-allocated.
*/

void DataProcessingGui::on_analysisFileSelectButton_clicked()
{
}

//-----------------------------------------------------------------------------
/** @brief Extract and Combine Raw Records to CSV.

Raw records are combined into single records for each time interval, and written
to a csv file. Format suitable for spreadsheet analysis.

@param[in] QDateTime start time.
@param[in] QDateTime end time.
@param[in] QFile* input file.
@param[in] QFile* output file.
*/

bool DataProcessingGui::combineRecords(QDateTime startTime, QDateTime endTime,
                                       QFile* inFile, QFile* outFile,bool header)
{
    int device1Voltage = -1;
    int device1Current = 0;
    int device2Voltage = -1;
    int device2Current = 0;
    int device3Voltage = -1;
    int device3Current = 0;
    int load1Voltage = -1;
    int load1Current = 0;
    int load2Voltage = -1;
    int load2Current = 0;
    int source1Voltage = -1;
    int source1Current = 0;
    int temperature = -1;
    QString switches;
    bool blockStart = false;
    QTextStream inStream(inFile);
    QTextStream outStream(outFile);
    if (header)
    {
        outStream << "Time,";
        outStream << "D1 I," << "D1 V,";
        outStream << "D2 I," << "D2 V,";
        outStream << "D3 I," << "D3 V,";
        outStream << "L1 I," << "L1 V," << "L2 I," << "L2 V," << "M1 I," << "M1 V,";
        outStream << "Temp," << "Switches";
        outStream << "\n\r";
    }
    QDateTime time = startTime;
    while (! inStream.atEnd())
    {
        if  (time > endTime) break;
      	QString lineIn = inStream.readLine();
        if (lineIn.size() <= 1) break;
        QStringList breakdown = lineIn.split(",");
        int size = breakdown.size();
        if (size <= 0) break;
        QString firstText = breakdown[0].simplified();
        QString secondText;
        QString thirdText;
        int secondField = -1;
        if (size > 1)
        {
            secondText = breakdown[1].simplified();
            secondField = secondText.toInt();
        }
        int thirdField = -1;
        if (size > 2)
        {
            thirdText = breakdown[2].simplified();
            thirdField = thirdText.toInt();
        }
        if (size > 1)
        {
// Find and extract the time record
            if (firstText == "pH")
            {
                time = QDateTime::fromString(breakdown[1].simplified(),Qt::ISODate);
                if ((blockStart) && (time > startTime))
                {
                    outStream << timeRecord << ",";
                    outStream << (float)device1Current/256 << ",";
                    outStream << (float)device1Voltage/256 << ",";
                    outStream << (float)device2Current/256 << ",";
                    outStream << (float)device2Voltage/256 << ",";
                    outStream << (float)device3Current/256 << ",";
                    outStream << (float)device3Voltage/256 << ",";
                    outStream << (float)load1Voltage/256 << ",";
                    outStream << (float)load1Current/256 << ",";
                    outStream << (float)load2Voltage/256 << ",";
                    outStream << (float)load2Current/256 << ",";
                    outStream << (float)source1Voltage/256 << ",";
                    outStream << (float)source1Current/256 << ",";
                    outStream << (float)temperature/256 << ",";
                    outStream << switches << ",";
                    outStream << "\n\r";
                }
                timeRecord = breakdown[1].simplified();
                blockStart = true;
            }
            if (firstText == "dB1")
            {
                device1Current = secondField;
                device1Voltage = thirdField;
            }
            if (firstText == "dB2")
            {
                device2Current = secondField;
                device2Voltage = thirdField;
            }
            if (firstText == "dB3")
            {
                device3Current = secondField;
                device3Voltage = thirdField;
            }
            if (firstText == "dB4")
            {
                load1Voltage = secondField;
                load1Current = thirdField;
            }
            if (firstText == "dB5")
            {
                load2Voltage = secondField;
                load2Current = thirdField;
            }
            if (firstText == "dB8")
            {
                source1Voltage = secondField;
                source1Current = thirdField;
            }
            if (firstText == "dT")
            {
                temperature = secondField;
            }
// Switch control bits - three 2-bit fields: device number for each of
// load1, load2 and source.
            if (firstText == "ds")
            {
                switches.clear();
                uint load1device = (secondField >> 0) & 0x03;
                QString load1deviceText;
                load1deviceText.setNum(load1device);
                if (load1device > 0) switches.append(" ").append(load1deviceText);
                else switches.append(" 0");
                uint load2device = (secondField >> 2) & 0x03;
                QString load2deviceText;
                load2deviceText.setNum(load2device);
                if (load2device > 0) switches.append(" ").append(load2deviceText);
                else switches.append(" 0");
                uint sourcedevice = (secondField >> 4) & 0x03;
                QString sourcedeviceText;
                sourcedeviceText.setNum(sourcedevice);
                if (sourcedevice > 0) switches.append(" ").append(sourcedeviceText);
                else switches.append(" 0");
            }
        }
    }
    return inStream.atEnd();
}

//-----------------------------------------------------------------------------
/** @brief Seek First Time Record.

The input file is searched record by record until the first time record is
found.

@param[in] QFile* input file.
@returns QDateTime time of first time record. Null if not found.
*/

QDateTime DataProcessingGui::findFirstTimeRecord(QFile* inFile)
{
    QDateTime time;
    QTextStream inStream(inFile);
    while (! inStream.atEnd())
    {
      	QString lineIn = inStream.readLine();
        if (lineIn.size() <= 1) break;
        QStringList breakdown = lineIn.split(",");
        int size = breakdown.size();
        if (size <= 0) break;
        QString firstText = breakdown[0].simplified();
// Find and extract the time record
        if ((size > 1) && (firstText == "pH"))
        {
            time = QDateTime::fromString(breakdown[1].simplified(),Qt::ISODate);
            break;
        }
    }
    return time;
}

//-----------------------------------------------------------------------------
/** @brief Open a Data File for Writing.

This is called from other action functions. The file is requested in a file
dialogue and opened. The function aborts if the file exists.

@returns true if file successfully created and opened.
*/

bool DataProcessingGui::openSaveFile()
{
    if (! saveFile.isEmpty())
    {
        displayErrorMessage("A save file is already open - close first");
        return false;
    }
    QString filename = QFileDialog::getSaveFileName(this,
                        "Save csv Data",
                        QString(),
                        "Comma Separated Variables (*.csv)",0,0);
    if (filename.isEmpty()) return false;
    if (! filename.endsWith(".csv")) filename.append(".csv");
    QFileInfo fileInfo(filename);
    saveDirectory = fileInfo.absolutePath();
    saveFile = saveDirectory.filePath(filename);
    outFile = new QFile(saveFile);             // Open file for output
    if (! outFile->open(QIODevice::WriteOnly))
    {
        displayErrorMessage("Could not open the output file");
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
/** @brief Scan the data file

Look for start and end times and record types. Obtain the current zeros from
records that have isolated operational status.

*/

void DataProcessingGui::scanFile(QFile* inFile)
{
    if (! inFile->isOpen()) return;
    QTextStream inStream(inFile);
    QDateTime startTime, endTime;
    while (! inStream.atEnd())
    {
      	QString lineIn = inStream.readLine();
        if (lineIn.size() <= 1) break;
        QStringList breakdown = lineIn.split(",");
        int length = breakdown.size();
        if (length <= 0) break;
        QString firstText = breakdown[0].simplified();
        QString secondText = breakdown[1].simplified();
        if ((firstText == "pH") && (length > 1))
        {
            QDateTime time = QDateTime::fromString(secondText,Qt::ISODate);
            if (startTime.isNull()) startTime = time;
            endTime = time;
        }
    }
    if (! startTime.isNull()) DataProcessingMainUi.startTime->setDateTime(startTime);
    if (! endTime.isNull()) DataProcessingMainUi.endTime->setDateTime(endTime);
}

//-----------------------------------------------------------------------------
/** @brief Print an error message.

*/

void DataProcessingGui::displayErrorMessage(QString message)
{
    DataProcessingMainUi.errorMessageLabel->setText(message);
}

//-----------------------------------------------------------------------------
/** @brief Message box for output file exists.

Checks the existence of a specified file and asks for decisions about its
use: abort, overwrite, or append.

@param[in] QString filename: name of output file.
@param[out] bool* append: true if append was selected.
@returns true if abort was selected.
*/

bool DataProcessingGui::outfileMessage(QString filename, bool* append)
{
    QString saveFile = saveDirectory.filePath(filename);
// If report filename exists, decide what action to take.
// Build a message box with options
    if (QFile::exists(saveFile))
    {
        QMessageBox msgBox;
        msgBox.setText(QString("A previous save file ").append(filename).
                        append(" exists."));
// Overwrite the existing file
        QPushButton *overwriteButton = msgBox.addButton(tr("Overwrite"),
                         QMessageBox::AcceptRole);
// Append to the existing file
        QPushButton *appendButton = msgBox.addButton(tr("Append"),
                         QMessageBox::AcceptRole);
// Quit altogether
        QPushButton *abortButton = msgBox.addButton(tr("Abort"),
                         QMessageBox::AcceptRole);
        msgBox.exec();
        if (msgBox.clickedButton() == overwriteButton)
        {
            QFile::remove(saveFile);
        }
        else if (msgBox.clickedButton() == abortButton)
        {
            return true;
        }
// Don't write the header into the appended file
        else if (msgBox.clickedButton() == appendButton)
        {
            *append = true;
        }
    }
    return false;
}

