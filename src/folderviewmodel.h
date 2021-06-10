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

#ifndef FOLDERVIEWMODEL_H
#define FOLDERVIEWMODEL_H

#include <QStandardItemModel>

class FolderNode
{
public:
    QString folderName;
    QList<FolderNode*> children;
    bool isChecked() const { return checked; }
    void setChecked( bool set ) { checked = set; }
    void setRoot(QString root) { folderRoot = root; }
    QString getRoot() {return folderRoot;}

private:
    bool checked;
    QString folderRoot;
};

class FolderViewModel : public QStandardItemModel
{
    Q_OBJECT
public:
    FolderViewModel();
    void addItem(QString volume, QString root, QString folderPath);
    void removeItem(QString volume, QString folderPath);

private:
//    QSet<QString> volumes;
    QMap<QString, int> folders;
    QStandardItem* rootItem;
    FolderNode* rootFolder;

};

#endif // FOLDERVIEWMODEL_H
