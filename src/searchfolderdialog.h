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

#ifndef SEARCHFOLDERDIALOG_H
#define SEARCHFOLDERDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class SearchFolderDialog;
}

class SearchFolderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchFolderDialog(QWidget *parent = nullptr);
    ~SearchFolderDialog();

public slots:
    // QDialog interface
    void accept();
    void reject();

signals:
    void searchFolderAdded(QString folder);
    void searchFolderRemoved(QString folder);

private:
    Ui::SearchFolderDialog *ui;
    QList<QString> searchFolders;
    QSettings settings;
    void addNewClicked();
    void removeClicked();
    void selectionChanged();
};

#endif // SEARCHFOLDERDIALOG_H
