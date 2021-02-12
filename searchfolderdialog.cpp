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

#include "searchfolderdialog.h"
#include "ui_searchfolderdialog.h"

#include <QFileDialog>

SearchFolderDialog::SearchFolderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchFolderDialog)
{
    ui->setupUi(this);

    settings.setDefaultFormat(QSettings::IniFormat);
    auto foldersFromList = settings.value("SearchFolders").value<QList<QString>>();
    searchFolders.append(foldersFromList);
    ui->searchFoldersWidget->addItems(foldersFromList);
    connect(ui->addNewButton, &QPushButton::clicked, this, &SearchFolderDialog::addNewClicked);
    connect(ui->removeSelectedButton, &QPushButton::clicked, this, &SearchFolderDialog::removeClicked);
    connect(ui->searchFoldersWidget, &QListWidget::itemSelectionChanged, this, &SearchFolderDialog::selectionChanged);
}

SearchFolderDialog::~SearchFolderDialog()
{
    delete ui;
}

void SearchFolderDialog::addNewClicked()
{
    auto OutputFolder = QFileDialog::getExistingDirectory(0, ("Select Output Folder"), QDir::homePath());

    if (OutputFolder != "" && !searchFolders.contains(OutputFolder))
    {
        // add the new folder to our folders list
        searchFolders.append(OutputFolder);
        settings.setValue("SearchFolders", searchFolders);
        ui->searchFoldersWidget->addItem(OutputFolder);
        emit SearchFolderAdded(OutputFolder);
    }
}

void SearchFolderDialog::removeClicked()
{
    auto selected = ui->searchFoldersWidget->selectedItems();
    for (auto& x : selected)
    {
        emit SearchFolderRemoved(x->text());
    }
    qDeleteAll(selected);
    QStringList folderList;

    int count = ui->searchFoldersWidget->count();
    for (int i = 0; i < count; ++i)
    {
        folderList.append(ui->searchFoldersWidget->item(i)->text());
    }
    settings.setValue("SearchFolders", folderList);
    searchFolders.clear();
    searchFolders.append(folderList);
}

void SearchFolderDialog::selectionChanged()
{
    bool itemSelected = ui->searchFoldersWidget->selectedItems().count() > 0;
    ui->removeSelectedButton->setDisabled(!itemSelected);
}

void SearchFolderDialog::accept()
{
    QDialog::accept();
}

void SearchFolderDialog::reject()
{
    QDialog::reject();
}
