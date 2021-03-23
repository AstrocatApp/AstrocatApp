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
#include "newfileprocessor.h"
#include "searchfolderdialog.h"
#include "sortfilterproxymodel.h"

#include <QAbstractItemModelTester>
#include <QFileInfo>
#include <QMainWindow>
#include <QThread>
#include <QItemSelection>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void initialize();

public slots:
    void newFileFound(const QFileInfo fileInfo);
    void searchFolderRemoved(const QString folder);

signals:
    void crawl(QString rootFolder);
    void deleteAstrofilesInFolder(const QString fullPath);
    void dbAddTags(const AstroFileImage& astroFileImage);
    void dbAddThumbnail(const AstroFileImage& astroFileImage, const QImage& image);
    void initializeFileRepository();
    void loadModelFromDb();
    void loadModelIntoViewModel(const QList<AstroFileImage> &files);
    void resetModel();

    void extractTags(const AstroFileImage& astroFileImage);
    void extractThumbnail(const AstroFileImage& astroFileImage);
    void processNewFile(const QFileInfo& fileInfo);

    void itemModelAddTags(const AstroFileImage& astroFileImage);
    void itemModelAddThumbnail(const AstroFileImage& astroFileImage);
    void insertAstrofileImage(const AstroFileImage& afi);

private slots:
    void on_imageSizeSlider_valueChanged(int value);
    void on_actionFolders_triggered();
    void handleSelectionChanged(QItemSelection selection);
    void modelLoadedFromDb(const QList<AstroFileImage> &files);

    void tagsExtracted(const AstroFileImage& astroFileImage, const QMap<QString, QString>& tags);
    void thumbnailExtracted(const AstroFileImage& astroFileImage, const QImage& img);

    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    bool isInitialized;

    QThread* folderCrawlerThread;
    FolderCrawler* folderCrawlerWorker;

    QThread* fileRepositoryThread;
    FileRepository* fileRepositoryWorker;

    QThread* newFileProcessorThread;
    NewFileProcessor* newFileProcessorWorker;

    FileViewModel* fileViewModel;
    SortFilterProxyModel* sortFilterProxyModel;

    SearchFolderDialog searchFolderDialog;
    QAbstractItemModelTester* tester;
    FilterWidget* filterWidget;

    QImage makeThumbnail(const QImage& image);
    void cleanUpWorker(QThread* thread);
    void clearDetailLabels();
    void crawlAllSearchFolders();
};
#endif // MAINWINDOW_H
