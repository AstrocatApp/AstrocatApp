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
#include "filterwidget.h"
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
    void newFileFound(const QFileInfo fileInfo);
    void getAstroFileFinished(const AstroFile astroFile);
    void getThumbnailFinished(const AstroFile& astroFile, const QPixmap& pixmap);
    void processFitsFileFinished(const AstroFile astroFile, const QImage& img, long nX, long nY );
    void searchFolderRemoved(const QString folder);
signals:
    void crawl(QString rootFolder);
    void getAstroFile(QString fullPath);
    void getAllAstroFiles();
    void getAllAstroFileTags();
    void insertAstroFile(AstroFile astroFile);
    void deleteAstrofilesInFolder(const QString fullPath);
    void dbAddTags(const AstroFile& astroFile);
    void dbAddThumbnail(const AstroFile& astroFile, const QImage& image);
    void dbGetThumbnails();
    void initializeFileRepository();
    void processFitsFile(const AstroFile& astroFile);
private slots:
    void on_pushButton_clicked();
    void on_imageSizeSlider_valueChanged(int value);
    void getAllAstroFilesFinished(const QList<AstroFile>&);
    void getAllAstroFileTagsFinished(const QMap<QString, QSet<QString>>&);

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

    FilterWidget* filterWidget;
};
#endif // MAINWINDOW_H
