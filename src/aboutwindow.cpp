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

#include <QDirIterator>
#include <QFile>

AboutWindow::AboutWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutWindow)
{
    readLicenseFromResource();

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
    Q_UNUSED(previous);

    QString selected = current->text();
    QString lic = _licenses[selected];

    ui->textBrowser->setText(lic);
}

void AboutWindow::readLicenseFromResource()
{
    QDirIterator it(":Licenses/resources/Licenses/", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString resourcePath = it.next();
        QFile licFile(resourcePath);
        licFile.open(QIODevice::ReadOnly);
        QString lic = licFile.readAll();
        licFile.close();

        QString fileName = it.fileInfo().baseName();
        _licenses.insert(fileName, lic);
    }
}
