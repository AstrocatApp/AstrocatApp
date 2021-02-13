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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "astrofile.h"
#include "filerepository.h"
#include "fileviewmodel.h"
#include "fitsprocessor.h"
#include "foldercrawler.h"
#include "searchfolderdialog.h"
#include "sortfilterproxymodel.h"

#include <QAbstractItemModelTester>
#include <QFileInfo>
#include <QMainWindow>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void NewFileFound(const QFileInfo fileInfo);
    void GetAstroFileFinished(const AstroFile astroFile);
    void GetThumbnailFinished(const AstroFile& astroFile, const QPixmap& pixmap);
    void ProcessFitsFileFinished(const AstroFile astroFile, const QImage& img, long nX, long nY );
    void SearchFolderRemoved(const QString folder);
signals:
    void Crawl(QString rootFolder);
    void GetAstroFile(QString fullPath);
    void GetAllAstroFiles();
    void GetAllAstroFileTags();
    void InsertAstroFile(AstroFile astroFile);
    void DeleteAstrofilesInFolder(const QString fullPath);
    void DbAddTags(const AstroFile& astroFile);
    void DbAddThumbnail(const AstroFile& astroFile, const QImage& image);
    void DbGetThumbnails();
    void InitializeFileRepository();
    void ProcessFitsFile(const AstroFile& astroFile);
private slots:
    void on_pushButton_clicked();
    void on_imageSizeSlider_valueChanged(int value);
    void GetAllAstroFilesFinished(const QList<AstroFile>&);
    void GetAllAstroFileTagsFinished(const QMap<QString, QSet<QString>>&);

    void on_actionFolders_triggered();

private:
    Ui::MainWindow *ui;

    QThread* folderCrawlerThread;
    FolderCrawler* folderCrawlerWorker;

    QThread* fileRepositoryThread;
    FileRepository* fileRepositoryWorker;

    QThread* fitsProcessorThread;
    FitsProcessor* fitsProcessorWorker;

    FileViewModel* fileViewModel;
    SortFilterProxyModel* sortFilterProxyModel;

    SearchFolderDialog searchFolderDialog;

    QImage MakeThumbnail(const QImage& image);
    QAbstractItemModelTester* tester;
};
#endif // MAINWINDOW_H
