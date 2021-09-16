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

#ifndef IMPORTFILEDIALOG_H
#define IMPORTFILEDIALOG_H

#include <QAbstractButton>
#include <QDialog>

namespace Ui {
class ImportFileDialog;
}

class ImportFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportFileDialog(QWidget *parent = nullptr);
    ~ImportFileDialog();

public slots:
    void IncrementActiveFoldersCrawling();
    void IncrementTotalFoldersCrawled();
    void IncrementTotalFilesImported();
    void IncrementTotalFilesAttempted();
    void IncrementTotalFilesFailedToProcess();
    void IncrementTotalFilesAlreadyInCatalog();
    void SetQueueSize(int value);
    void ResetActiveFoldersCrawling();
    void ResetTotalFoldersCrawled();
    void ResetTotalFilesImported();
    void ResetTotalFilesAttempted();
    void ResetTotalFilesFailedToProcess();
    void ResetTotalFilesAlreadyInCatalog();
    void ResetAllValues();
    void ResetQueueSize();

signals:
    void PauseClicked(QAbstractButton* button);

private:
    Ui::ImportFileDialog *ui;

    int activeFoldersCrawling;
    int totalFoldersCrawled;
    int totalFilesImported;
    int totalFilesAttempted;
    int totalFilesFailedToProcess;
    int totalFilesAlreadyInCatalog;
    int queueSize;
};

#endif // IMPORTFILEDIALOG_H
