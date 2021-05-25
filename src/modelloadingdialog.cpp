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

#include "modelloadingdialog.h"
#include "ui_modelloadingdialog.h"

ModelLoadingDialog::ModelLoadingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ModelLoadingDialog)
{
    ui->setupUi(this);
    this->ui->statusLabel->setText("Loading images");
    this->ui->progressBar->setValue(5);
}

ModelLoadingDialog::~ModelLoadingDialog()
{
    delete ui;
}

void ModelLoadingDialog::modelLoadingFromDbGotAstrofiles()
{
    this->ui->statusLabel->setText("Loading Tags");
    this->ui->progressBar->setValue(20);
}

void ModelLoadingDialog::modelLoadingFromDbGotTag()
{
    this->ui->statusLabel->setText("Loading Thumbnails");
    this->ui->progressBar->setValue(40);
}

void ModelLoadingDialog::modelLoadingFromDbGotThumbnails()
{
    this->ui->statusLabel->setText("Loaded Thumbnails");
    this->ui->progressBar->setValue(60);
}

void ModelLoadingDialog::modelLoaded()
{
    this->ui->statusLabel->setText("Drawing Thumbnails");
    this->ui->progressBar->setValue(80);
}

void ModelLoadingDialog::closeWindow()
{
    this->ui->progressBar->setValue(100);
    this->close();
}
