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

#include "importfiledialog.h"
#include "ui_importfiledialog.h"

ImportFileDialog::ImportFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportFileDialog)
{
    activeFoldersCrawling = 0;
    totalFoldersCrawled = 0;
    totalFilesImported = 0;
    totalFilesAttempted = 0;
    totalFilesFailedToProcess = 0;
    totalFilesAlreadyInCatalog = 0;

    ui->setupUi(this);
}

ImportFileDialog::~ImportFileDialog()
{
    delete ui;
}

void ImportFileDialog::IncrementActiveFoldersCrawling()
{
    activeFoldersCrawling++;
}

void ImportFileDialog::IncrementTotalFoldersCrawled()
{
    totalFoldersCrawled++;
}

void ImportFileDialog::IncrementTotalFilesImported()
{
    totalFilesImported++;
    ui->filesImportedLabel->setText(QString::number(totalFilesImported));
}

void ImportFileDialog::IncrementTotalFilesAttempted()
{
    totalFilesAttempted++;
    ui->filesFoundLabel->setText(QString::number(totalFilesAttempted));
}

void ImportFileDialog::IncrementTotalFilesFailedToProcess()
{
    totalFilesFailedToProcess++;
    ui->filesFailedLabel->setText(QString::number(totalFilesFailedToProcess));
}

void ImportFileDialog::IncrementTotalFilesAlreadyInCatalog()
{
    totalFilesAlreadyInCatalog++;
    ui->filesInCatalogLabel->setText(QString::number(totalFilesAlreadyInCatalog));
}

void ImportFileDialog::ResetActiveFoldersCrawling()
{
    activeFoldersCrawling = 0;
}

void ImportFileDialog::ResetTotalFoldersCrawled()
{
    totalFoldersCrawled = 0;
}

void ImportFileDialog::ResetTotalFilesImported()
{
    totalFilesImported = 0;
    ui->filesImportedLabel->setText(QString::number(totalFilesImported));
}

void ImportFileDialog::ResetTotalFilesAttempted()
{
    totalFilesAttempted = 0;
    ui->filesFoundLabel->setText(QString::number(totalFilesAttempted));
}

void ImportFileDialog::ResetTotalFilesFailedToProcess()
{
    totalFilesFailedToProcess = 0;
    ui->filesFailedLabel->setText(QString::number(totalFilesFailedToProcess));
}

void ImportFileDialog::ResetTotalFilesAlreadyInCatalog()
{
    totalFilesAlreadyInCatalog = 0;
    ui->filesInCatalogLabel->setText(QString::number(totalFilesAlreadyInCatalog));
}

void ImportFileDialog::ResetAllValues()
{
    ResetActiveFoldersCrawling();
    ResetTotalFoldersCrawled();
    ResetTotalFilesImported();
    ResetTotalFilesAttempted();
    ResetTotalFilesFailedToProcess();
    ResetTotalFilesAlreadyInCatalog();
}
