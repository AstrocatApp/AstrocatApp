/*
    MIT License

    Copyright (c) 2021 Astrocat.App

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "aboutwindow.h"
#include "ui_aboutwindow.h"

#include <QFile>

AboutWindow::AboutWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutWindow)
{
    cfitsioLicense  = readLicenseFromResource("CFITSIO.lic");
    cminpackLicense = readLicenseFromResource("CMINPACK.lic");
    lcmsLicense     = readLicenseFromResource("LCMS.lic");
    lz4License      = readLicenseFromResource("LZ4.lic");
    pclLicense      = readLicenseFromResource("PCL.lic");
    qtLicense       = readLicenseFromResource("QT.lic");
    rfc6234License  = readLicenseFromResource("RFC6234.lic");
    zlibLicense     = readLicenseFromResource("ZLIB.lic");

    QString ver = QCoreApplication::applicationVersion();
    QString version = QString("Version: %1").arg(CURRENT_APP_VERSION);
    ui->setupUi(this);

    ui->versionLabel->setText(version);
    ui->listWidget->setCurrentRow(0);
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    QString lic;
    QString selected = current->text();
    if (selected == "CFITSIO")
        lic = cfitsioLicense;
    else if (selected == "CMINPACK")
        lic = cminpackLicense;
    else if (selected == "LCMS")
        lic = lcmsLicense;
    else if (selected == "LZ4")
        lic = lz4License;
    else if (selected == "PCL")
        lic = pclLicense;
    else if (selected == "QT")
        lic = qtLicense;
    else if (selected == "RFC6234")
        lic = rfc6234License;
    else if (selected == "ZLIB")
        lic = zlibLicense;

    ui->textBrowser->setText(lic);
}

QString AboutWindow::readLicenseFromResource(QString library)
{
    qDebug() <<"Reading License: " + library;
    QString licTxt(":Licenses/resources/Licenses/" + library);
    QFile licFile(licTxt);
    licFile.open(QIODevice::ReadOnly);
    QString lic = licFile.readAll();
    licFile.close();
    return lic;
}
