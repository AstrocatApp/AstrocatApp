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
#include "newfileprocessor.h"
#include "searchfolderdialog.h"
#include "sortfilterproxymodel.h"
#include "filterview.h"
#include "catalog.h"
#include "fileprocessfilter.h"
#include "thumbnailcache.h"
#include "modelloadingdialog.h"
#include "importfiledialog.h"
#include "fileimporter.h"
#include "dbservice.h"

#include <QFileInfo>
#include <QMainWindow>
#include <QThread>
#include <QItemSelection>
#include <QLabel>
#include <QPair>

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
    void cancelPendingOperations();

public slots:
    void searchFolderAdded(const QString folder);
    void searchFolderRemoved(const QString folder);    
    void importFiles(const QList<QUrl> folders);

signals:
    void crawl(QString rootFolder);
    void deleteAstrofilesInFolder(const QString fullPath);
    void dbAddAstroFile(const AstroFile& astroFile);
    void dbAddTags(const AstroFile& astroFile);
    void dbAddThumbnail(const AstroFile& astroFile, const QImage& image);
    void dbUpdateProcessStatus(const AstroFile& astroFile);
    void initializeFileRepository();
    void loadModelFromDb();
    void loadModelIntoViewModel(const QList<AstroFile> &files);
    void resetModel();
    void dbGetDuplicates();
    void newAstroFileImported();

    void extractTags(const AstroFile& astroFile);
    void extractThumbnail(const AstroFile& astroFile);
    void processNewFile(const QFileInfo& fileInfo);

    void catalogAddAstroFile(const AstroFile &file);
    void catalogAddAstroFiles(const QList<AstroFile> &files);

    void removeAstroFileFromCatalog(const AstroFile& file);

    void loadFilterStats(const QString, QList<QPair<QString, QString>>& filters);
    void loadFileExtensionStats(const QString fileExtension, QList<QPair<QString, QString>>& filters);
    void loadAstroFilesFromDb(const QString fileExtension, QList<QPair<QString, QString>>& filters);
    void resetFilters();

private slots:
    void imageSizeSlider_valueChanged(int value);
    void actionFolders_triggered();
    void handleSelectionChanged(QItemSelection selection);
    void modelLoadedFromDb(const QList<AstroFile> &files);

    void astroFileProcessed(const AstroFile& astroFile);
    void processingCancelled(const QFileInfo& fileInfo);
    void processQueued(const QFileInfo &fileInfo);

    void actionAbout_triggered();
    void setWatermark(bool shoudSet);
    void catalogAction_triggered();
    void importAction_triggered();
    void settingsAction_triggered();

    void rowsAddedToModel(const QModelIndex &parent, int first, int last);
    void rowsRemovedFromModel(const QModelIndex &parent, int first, int last);
    void modelReset();
    void itemAddedToSortFilterView(int numberAdded);
    void itemRemovedFromSortFilterView(int numberRemoved);
    void itemContextMenuRequested(const QPoint &pos);

    void reveal();
    void remove();
    void duplicatesButton_clicked();

    void dbFailedToOpen(const QString message);
    void dbAstroFileAdded(const AstroFile& astroFile);
//    void dbAstroFileDeleted(const AstroFile& astroFile);
    void importCancelled();
    void importPaused(QAbstractButton* button);
    void filesDropped(const QList<QUrl> &folders);
    void DatabaseQueueLength(int length);

    void FileImporterFinished();

    void addedFilterQuery(QString filterKey, QString filterValue);
    void removedFilterQuery(QString filterKey, QString filterValue);
    void addedFileExtensionQuery(QString fileExtension);
    void removedFileExtensionQuery(QString fileExtension);

private:
    Ui::MainWindow *ui;
    bool isInitialized;

    DbService* dbService;

    FileViewModel* fileViewModel;
    SortFilterProxyModel* sortFilterProxyModel;

//    SearchFolderDialog searchFolderDialog;
    FilterView* filterView;

    ImportFileDialog importFileDialog;

//    QImage makeThumbnail(const QImage& image);
    void cleanUpWorker(QThread* thread);
    void clearDetailLabels();

    bool shouldShowWatermark = true;
    const QString DEFAULT_WATERMARK_MESSAGE = "Drop Files or Folders here to import.";
    QString _watermarkMessage = DEFAULT_WATERMARK_MESSAGE;

    int numberOfItems = 0;
    int numberOfVisibleItems = 0;
    int numberOfSelectedItems = 0;
    int numberOfActiveJobs = 0;

    QLabel numberOfItemsLabel;
    QLabel numberOfVisibleItemsLabel;
    QLabel numberOfSelectedItemsLabel;
    QLabel numberOfActiveJobsLabel;
    QLabel dbQueueStatus;

    QAction *revealAct;
    QAction *removeAct;
    void createActions();
    void showImportDialog();

    QThread* catalogThread;
    Catalog* catalog;

    ThumbnailCache thumbnailCache;
    ModelLoadingDialog* loading;

    void disableDropFiles();
    void enableDropFiles();

    FileImporter* fileImporter;
    void createFileImporter();

    QList<QPair<QString, QString>> filters;
    QString fileExtensionFilter;
protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
};
#endif // MAINWINDOW_H
