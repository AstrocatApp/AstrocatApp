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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "searchfolderdialog.h"
#include "aboutwindow.h"
#include "mock_newfileprocessor.h"
#include "catalog.h"
#include "mock_foldercrawler.h"
#include "modelloadingdialog.h"

#include <QContextMenuEvent>
#include <QMessageBox>
#include <QPainter>
#include <QDesktopServices>
#include <QProcess>
#include <QPixmapCache>
#include <QDir>
#include <QToolBar>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      isInitialized(false)
{
    ui->setupUi(this);

    fileImporter = nullptr;
    catalogThread = new QThread(this);
    catalogThread->setObjectName("CatalogThread");
    catalog = new Catalog;
    catalog->moveToThread(catalogThread);

    fileViewModel = new FileViewModel(ui->astroListView);
    fileViewModel->setCatalog(catalog);
    sortFilterProxyModel = new SortFilterProxyModel(ui->astroListView);
    sortFilterProxyModel->setSourceModel(fileViewModel);
    sortFilterProxyModel->setDynamicSortFilter(false);
    ui->astroListView->setViewMode(QListView::IconMode);
    ui->astroListView->setResizeMode(QListView::Adjust);
    ui->astroListView->setModel(sortFilterProxyModel);
    ui->astroListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->astroListView->setUniformItemSizes(true);
    ui->astroListView->setIconSize(QSize(200,200)*0.9);

//    ui->astroListView->setAcceptDrops(true);
//    ui->astroListView->viewport()->setAcceptDrops(true);
    ui->astroListView->setDropIndicatorShown(false);
    ui->astroListView->setDragDropMode(QAbstractItemView::DropOnly);

    ui->astroListView->setStyleSheet(
                "QListView {"
                    "background-color: #232323;"
                    "color: white;"
                    "}"
                "QListView::item {"
                    "background-color: #232323;"
                    "color: white;"
                    "}"
                "QListView::item:selected {"
                    "border: 1px solid #6a6ea9;"
                    "background-color: #232323;"
                    "color: white;"
                    "}"
                "QListView::item:selected:active {"
                    "border: 1px solid #6a6ea9;"
                    "background-color: #232323;"
                    "color: white;"
                    "}"
                );

    QItemSelectionModel *selectionModel = ui->astroListView->selectionModel();
    filterView = new FilterView(ui->scrollAreaWidgetContents_2);
    thumbnailCache.start();

    ui->statusbar->setStyleSheet(
                "QStatusBar {color: white; background-color: #232323;}"
                "QWidget {color: white;}"
                );
    ui->statusbar->addPermanentWidget(&numberOfItemsLabel);
    ui->statusbar->addPermanentWidget(&numberOfVisibleItemsLabel);
    ui->statusbar->addPermanentWidget(&numberOfSelectedItemsLabel);
    ui->statusbar->addPermanentWidget(&dbQueueStatus);
    //    ui->statusbar->addPermanentWidget(&numberOfActiveJobsLabel);

    ui->splitter->setSizes({50, 1000});

    //+++ Custom Toolbar
//    QToolBar *toolbar = addToolBar("main toolbar");
//    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
//    toolbar->setStyleSheet("QToolButton{font-size: 20px;}");
//    QAction* catalogAction = toolbar->addAction("Catalog");
//    toolbar->addSeparator();
//    QAction* importAction = toolbar->addAction("Import");
//    toolbar->addSeparator();
//    QAction* settingsAction = toolbar->addAction("Settings");
//    toolbar->addSeparator();
//    QAction* lastImportAction = toolbar->addAction("Last Imported");
    //--- Custom Toolbar

    ui->astroListView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    createActions();
    QPixmapCache::setCacheLimit(100*1024);

    loading = new ModelLoadingDialog(this);
    importFileDialog.setModal(false);
    importFileDialog.setWindowFlag(Qt::WindowStaysOnTopHint);

    dbService = new DbService();

//    connect(catalogAction,          &QAction::triggered,                                this,                   &MainWindow::catalogAction_triggered);
//    connect(importAction,           &QAction::triggered,                                this,                   &MainWindow::importAction_triggered);
//    connect(settingsAction,         &QAction::triggered,                                this,                   &MainWindow::settingsAction_triggered);
//    connect(lastImportAction,       &QAction::triggered,                                this,                   &MainWindow::settingsAction_triggered);

    connect(&importFileDialog,      &ImportFileDialog::rejected,                        this,                   &MainWindow::importCancelled);
    connect(&importFileDialog,      &ImportFileDialog::PauseClicked,                        this,                   &MainWindow::importPaused);

    connect(ui->imageSizeSlider,    &QSlider::valueChanged,                             this,                   &MainWindow::imageSizeSlider_valueChanged);
    connect(ui->astroListView,      &QWidget::customContextMenuRequested,               this,                   &MainWindow::itemContextMenuRequested);
    connect(ui->actionFolders,      &QAction::triggered,                                this,                   &MainWindow::actionFolders_triggered);
    connect(ui->actionAbout,        &QAction::triggered,                                this,                   &MainWindow::actionAbout_triggered);
    connect(this,                   &MainWindow::initializeFileRepository,              dbService,   &DbService::initialize);
    connect(this,                   &MainWindow::deleteAstrofilesInFolder,              dbService,   &DbService::deleteAstrofilesInFolder);
    connect(this,                   &MainWindow::loadModelFromDb,                       dbService,   &DbService::loadModel);
    connect(this,                   &MainWindow::dbAddAstroFile,                dbService,   &DbService::addAstrofile);
    connect(this,                   &MainWindow::loadFilterStats,                       dbService,   &DbService::loadFilterStats);
    connect(this,                   &MainWindow::loadFileExtensionStats,                       dbService,   &DbService::loadFileExtensionStats);
    connect(this,                   &MainWindow::loadAstroFilesFromDb,                       dbService,   &DbService::loadAstroFiles);
    connect(this,                   &MainWindow::dbGetDuplicates,                       dbService,   &DbService::getDuplicateFiles);
    connect(catalogThread,          &QThread::finished,                                 catalog,                &QObject::deleteLater);
    connect(catalog,                &Catalog::AstroFilesAdded,                          fileViewModel,          &FileViewModel::AddAstroFiles);
    connect(this,                   &MainWindow::newAstroFileImported,                  &importFileDialog,      &ImportFileDialog::IncrementTotalFilesImported);
    connect(catalog,                &Catalog::AstroFileUpdated,                         fileViewModel,          &FileViewModel::UpdateAstroFile);
    connect(this,                   &MainWindow::catalogAddAstroFile,                   catalog,                &Catalog::addAstroFile);
    connect(this,                   &MainWindow::catalogAddAstroFiles,                  catalog,                &Catalog::addAstroFiles);
    connect(this,                   &MainWindow::removeAstroFileFromCatalog,            dbService,   &DbService::deleteAstrofile);
    connect(dbService,   &DbService::astroFileAdded,                  this,                   &MainWindow::dbAstroFileAdded);
    connect(dbService,          &DbService::astroFileAdded,             filterView,          &FilterView::AstroFileImported);
    connect(dbService,   &DbService::astroFileDeleted,                  fileViewModel,          &FileViewModel::RemoveAstroFile);
    connect(dbService,   &DbService::astroFilesDeleted,                 fileViewModel,          &FileViewModel::RemoveAstroFiles);
    connect(dbService,   &DbService::modelLoaded,                       this,                   &MainWindow::modelLoadedFromDb);
    connect(dbService,   &DbService::dbFailedToInitialize,              this,                   &MainWindow::dbFailedToOpen);
    connect(dbService,   &DbService::thumbnailLoaded,                   fileViewModel,          &FileViewModel::addThumbnail);
    connect(dbService,   &DbService::astroFilesInFilter,                   sortFilterProxyModel,          &SortFilterProxyModel::setAstroFilesInFilter);
//    connect(&searchFolderDialog,    &SearchFolderDialog::searchFolderAdded,             this,                   &MainWindow::searchFolderAdded);
    connect(fileViewModel,          &FileViewModel::fileDropped,                        this,                   &MainWindow::filesDropped);
//    connect(&searchFolderDialog,    &SearchFolderDialog::searchFolderRemoved,           this,                   &MainWindow::searchFolderRemoved);
    connect(fileViewModel,          &FileViewModel::modelIsEmpty,                       this,                   &MainWindow::setWatermark);
    connect(fileViewModel,          &FileViewModel::rowsInserted,                       this,                   &MainWindow::rowsAddedToModel);
    connect(fileViewModel,          &FileViewModel::rowsRemoved,                        this,                   &MainWindow::rowsRemovedFromModel);
    connect(fileViewModel,          &FileViewModel::modelReset,                         this,                   &MainWindow::modelReset);
    connect(fileViewModel,          &FileViewModel::loadThumbnailFromDb,                &thumbnailCache,        &ThumbnailCache::enqueueLoadThumbnail);
    connect(&thumbnailCache,        &ThumbnailCache::dbLoadThumbnail,                   dbService,   &DbService::loadThumbnail);
    connect(this,             &MainWindow::resetFilters,                    sortFilterProxyModel,   &SortFilterProxyModel::resetFilters);

    connect(filterView,             &FilterView::addFilterQuery,                        this,   &MainWindow::addedFilterQuery);
    connect(filterView,             &FilterView::removeFilterQuery,                     this,   &MainWindow::removedFilterQuery);
    connect(filterView,             &FilterView::addAcceptedExtension,                  this,   &MainWindow::addedFileExtensionQuery);
    connect(filterView,             &FilterView::removeAcceptedExtension,               this,   &MainWindow::removedFileExtensionQuery);

    connect(filterView,             &FilterView::astroFileAdded,                        this,                   &MainWindow::itemAddedToSortFilterView);
    connect(filterView,             &FilterView::astroFileRemoved,                      this,                   &MainWindow::itemRemovedFromSortFilterView);
    connect(selectionModel,         &QItemSelectionModel::selectionChanged,             this,                   &MainWindow::handleSelectionChanged);
    connect(dbService,   &DbService::modelLoadingGotAstrofiles,         loading,                &ModelLoadingDialog::modelLoadingFromDbGotAstrofiles);
    connect(dbService,   &DbService::modelLoadingGotTags,               loading,                &ModelLoadingDialog::modelLoadingFromDbGotTag);
    connect(dbService,   &DbService::modelLoadingGotThumbnails,         loading,                &ModelLoadingDialog::modelLoadingFromDbGotThumbnails);
    connect(dbService,   &DbService::modelLoaded,                       loading,                &ModelLoadingDialog::modelLoaded);
    connect(catalog,                &Catalog::DoneAddingAstrofiles,                     loading,                &ModelLoadingDialog::closeWindow);
    connect(dbService,   &DbService::DatabaseQueueLength,                       this,                &MainWindow::DatabaseQueueLength);
    connect(dbService,   &DbService::FileExtensionStatsLoaded,                       filterView,                &FilterView::FileExtensionStatsLoaded);
    connect(dbService,   &DbService::FilterStatsLoaded,                       filterView,                &FilterView::FilterStatsLoaded);

    // Enable the tester during development and debugging. Disble before committing
//    tester = new QAbstractItemModelTester(fileViewModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
}

MainWindow::~MainWindow()
{
    cancelPendingOperations();

//    qDebug()<<"Cleaning up folderCrawlerThread";
//    cleanUpWorker(folderCrawlerThread);

//    qDebug()<<"Cleaning up newFileProcessorThread";
//    cleanUpWorker(newFileProcessorThread);

//    qDebug()<<"Cleaning up fileRepositoryThread";
//    cleanUpWorker(fileRepositoryThread);

    qDebug()<<"Cleaning up ThumbnailCache Thread";
//    cleanUpWorker(thumbnailCache);
    thumbnailCache.quit();

    qDebug()<<"Cleaning up fileViewModel";
    delete fileViewModel;

    qDebug()<<"Cleaning up catalogThread";
    cleanUpWorker(catalogThread);

//    db.close();
    qDebug()<<"Cleaning up ui";
    delete ui;
    qDebug()<<"Done Cleaning up.";
}

void MainWindow::cancelPendingOperations()
{
    if (fileImporter != nullptr)
    {
        fileImporter->CancelImport();
    }
    catalog->cancel();
    thumbnailCache.cancel();
//    fileFilter->cancel();
//    folderCrawlerWorker->cancel();
//    newFileProcessorWorker->cancel();
//    fileRepositoryWorker->cancel();
}

void MainWindow::initialize()
{
    if (isInitialized)
        return;
    catalogThread->start();

    emit initializeFileRepository();
    _watermarkMessage = "Loading Catalog...";
    setWatermark(true);

    loading->open();
    emit loadModelFromDb();

    isInitialized = true;
    emit dbGetDuplicates();

    emit loadFileExtensionStats(fileExtensionFilter, filters);
    emit loadFilterStats(fileExtensionFilter, filters);
//    emit loadAstroFilesFromDb(fileExtensionFilter, filters);
}

void MainWindow::cleanUpWorker(QThread* thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

void MainWindow::showImportDialog()
{
    importFileDialog.ResetAllValues();
    importFileDialog.show();
}

void MainWindow::importFiles(const QList<QUrl> folders)
{

}

void MainWindow::disableDropFiles()
{

}

void MainWindow::enableDropFiles()
{

}

void MainWindow::createFileImporter()
{
    fileImporter = new FileImporter(this);
    fileImporter->setCatalog(catalog);
    connect(fileImporter,          &FileImporter::AstroFileImported,              this,          &MainWindow::astroFileProcessed);
    connect(fileImporter,          &FileImporter::AstroFileImporting,                  this,                   &MainWindow::processQueued);
    connect(fileImporter,          &FileImporter::AstroFileIsInCatalog,                  &importFileDialog,      &ImportFileDialog::IncrementTotalFilesAlreadyInCatalog);
    connect(fileImporter,          &FileImporter::AstroFileFound,                          &importFileDialog,      &ImportFileDialog::IncrementTotalFilesAttempted);
    connect(fileImporter,          &FileImporter::ImportCanceled,                          this,      &MainWindow::FileImporterFinished);
    connect(fileImporter,          &FileImporter::ImportFinished,                          this,      &MainWindow::FileImporterFinished);
}

void MainWindow::searchFolderAdded(const QString folder)
{
    emit crawl(folder);
}

void MainWindow::searchFolderRemoved(const QString folder)
{
    // The source folder was removed by the user. We will need to remove all images in this source folder from the db.
    emit deleteAstrofilesInFolder(folder);
}

void MainWindow::imageSizeSlider_valueChanged(int value)
{
    auto currentIndex = ui->astroListView->currentIndex();

    if (!currentIndex.isValid())
        currentIndex = ui->astroListView->indexAt(QPoint(0,0));

    QPersistentModelIndex pIndex(currentIndex);

    // TODO: This call causes filterAcceptsRow() to be called in the sortFilterProxyModel. Investigate and see if we need to fix.
    fileViewModel->setCellSize(value);
    QSize size = QSize(4,4)*value*0.9;
    ui->astroListView->setIconSize(size);


    // TODO: This call causes filterAcceptsRow() to be called in the sortFilterProxyModel. Investigate and see if we need to fix.
//    auto scrollToIndex = sortFilterProxyModel->index(currentIndex.row(), currentIndex.column(), QModelIndex());
//    auto scrollToIndex = sortFilterProxyModel->mapFromSource(currentIndex);

    ui->astroListView->scrollTo(pIndex, QAbstractItemView::ScrollHint::EnsureVisible);
//    ui->astroListView->scrollTo(currentIndex, QAbstractItemView::ScrollHint::PositionAtTop);
//    ui->astroListView->scrollTo(currentIndex);
}

//QImage MainWindow::makeThumbnail(const QImage &image)
//{
//    QImage small =image.scaled( QSize(200, 200), Qt::KeepAspectRatio);
//    return small;
//}

void MainWindow::actionFolders_triggered()
{
//    searchFolderDialog.exec();
}

void MainWindow::actionAbout_triggered()
{
    AboutWindow about(this);
    about.exec();
}

void MainWindow::clearDetailLabels()
{
    ui->filenameLabel->clear();
    ui->objectLabel->clear();
    ui->insturmentLabel->clear();
    ui->filterLabel->clear();
    ui->dateLabel->clear();
    ui->bayerpatternLabel->clear();
    ui->exposureLabel->clear();
    ui->gainLabel->clear();
    ui->offsetLabel->clear();
    ui->raLabel->clear();
    ui->decLabel->clear();
    ui->temperatureLabel->clear();
    ui->fullPathLabel->clear();
    ui->imagesizeLabel->clear();
}

void MainWindow::handleSelectionChanged(QItemSelection selection)
{
    int numSelectedRows =  ui->astroListView->selectionModel()->selectedRows().count();
    if (numSelectedRows == 0)
    {
        clearDetailLabels();
        return;
    }

    if (numSelectedRows > 1)
    {
        clearDetailLabels();
        return;
    }

    if (selection.count() == 0)
    {
        clearDetailLabels();
        return;
    }

    if (selection[0].indexes().count() == 0)
    {
        clearDetailLabels();
        return;
    }

    QModelIndex index = selection[0].indexes()[0];

    auto xSize = sortFilterProxyModel->data(index, AstroFileRoles::ImageXSizeRole).toString();
    auto ySize = sortFilterProxyModel->data(index, AstroFileRoles::ImageYSizeRole).toString();

    ui->filenameLabel->setText(sortFilterProxyModel->data(index, Qt::DisplayRole).toString());
    ui->objectLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::ObjectRole).toString());
    ui->insturmentLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::InstrumentRole).toString());
    ui->filterLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::FilterRole).toString());
    ui->dateLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::DateRole).toString());
    ui->bayerpatternLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::BayerModeRole).toString());
    ui->exposureLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::ExposureRole).toString());
    ui->gainLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::GainRole).toString());
    ui->offsetLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::OffsetRole).toString());
    ui->raLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::RaRole).toString());
    ui->decLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::DecRole).toString());
    ui->temperatureLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::CcdTempRole).toString());
    ui->fullPathLabel->setText(sortFilterProxyModel->data(index, AstroFileRoles::FullPathRole).toString());

    if (! xSize.isEmpty() && ! ySize.isEmpty())
        ui->imagesizeLabel->setText(xSize+"x"+ySize);
}

void MainWindow::modelLoadedFromDb(const QList<AstroFile> &files)
{
    if (files.count() == 0)
    {
        _watermarkMessage = DEFAULT_WATERMARK_MESSAGE;
        setWatermark(true);
    }
    emit catalogAddAstroFiles(files);
}

void MainWindow::astroFileProcessed(const AstroFile &astroFile)
{
    QFileInfo fileInfo(astroFile.FullPath);
    if (catalog->shouldProcessFile(fileInfo) == RemovedFile)
    {
        // This file is not in the catalog anymore.
        numberOfActiveJobs--;
        ui->statusbar->showMessage(QString("ueue: %1").arg(numberOfActiveJobs));
        importFileDialog.SetQueueSize(numberOfActiveJobs);
        return;
    }

    // do not decrement numberOfActiveJobs yet. It will be decremented
    // after the db recorded it.
    emit dbAddAstroFile(astroFile);
    if (astroFile.processStatus == AstroFileFailedToProcess)
    {
        emit importFileDialog.IncrementTotalFilesFailedToProcess();
    }
    else
    {
        emit newAstroFileImported();
    }
}

void MainWindow::processingCancelled(const QFileInfo &fileInfo)
{
    Q_UNUSED(fileInfo);

    numberOfActiveJobs--;
    ui->statusbar->showMessage(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
    importFileDialog.SetQueueSize(numberOfActiveJobs);
}

void MainWindow::processQueued(const QFileInfo &fileInfo)
{
    Q_UNUSED(fileInfo);

    numberOfActiveJobs++;
    ui->statusbar->showMessage(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
    importFileDialog.SetQueueSize(numberOfActiveJobs);
}

void MainWindow::setWatermark(bool shouldSet)
{
    shouldShowWatermark = shouldSet;
    if (shouldSet)
    {
        QFont font;
        font.setPixelSize(24);

        QBrush brush;
        QPixmap pix(ui->astroListView->size());
        pix.fill(Qt::transparent);
        QPainter paint(&pix);
        paint.setFont(font);
        paint.setPen(QPen(QColor(Qt::GlobalColor::gray), Qt::SolidPattern));
        paint.drawText(ui->astroListView->frameRect(), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter, _watermarkMessage);
        brush.setTexture(pix);

        ui->astroListView->setStyleSheet("");
        QPalette palette;
        palette.setBrush(QPalette::Base, brush);
        ui->astroListView->setPalette(palette);
    }
    else
    {
        ui->astroListView->setPalette(QPalette());
    }
}

void MainWindow::catalogAction_triggered()
{
    qDebug()<<"Catalog Action";
}

void MainWindow::importAction_triggered()
{
//    actionFolders_triggered();
    importFileDialog.exec();
}

void MainWindow::settingsAction_triggered()
{
    qDebug()<<"Settings Action";
    // TODO: Implement here
}

void MainWindow::rowsAddedToModel(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);

    int numberAdded = last-first+1;
    this->numberOfItems += numberAdded;
    this->numberOfItemsLabel.setText(QString("Items: %1").arg(numberOfItems));
}

void MainWindow::rowsRemovedFromModel(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);

    int numberRemoved = last-first+1;
    this->numberOfItems -= numberRemoved;
    this->numberOfItemsLabel.setText(QString("Items: %1").arg(numberOfItems));
}

void MainWindow::modelReset()
{
    this->numberOfItems = 0;
    this->numberOfItemsLabel.setText(QString("Items: %1").arg(numberOfItems));
}

void MainWindow::itemAddedToSortFilterView(int numberAdded)
{
    this->numberOfVisibleItems+= numberAdded;
    this->numberOfVisibleItemsLabel.setText(QString("Shown Items: %1").arg(numberOfVisibleItems));
}

void MainWindow::itemRemovedFromSortFilterView(int numberRemoved)
{
    this->numberOfVisibleItems-=numberRemoved;
    this->numberOfVisibleItemsLabel.setText(QString("Shown Items: %1").arg(numberOfVisibleItems));
}

void MainWindow::itemContextMenuRequested(const QPoint &pos)
{
    auto index = ui->astroListView->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu menu(this);
    menu.addAction(revealAct);
    menu.addAction(removeAct);
    auto menuPos = ui->astroListView->viewport()->mapToGlobal(pos);
    menu.exec(menuPos);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    setWatermark(shouldShowWatermark);
}

void MainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    setWatermark(shouldShowWatermark);
}

void revealFile(QWidget* parent, const QString &pathToReveal) {

    // See http://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
    // for details

#if defined(Q_OS_WIN)
    QString path = QDir::toNativeSeparators(pathToReveal);
    QString param = QString("/select," + path);
    QProcess::startDetached("explorer.exe", QStringList(param));
#elif defined(Q_OS_MAC)
    Q_UNUSED(parent)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
            << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
            .arg(pathToReveal);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
            << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#else
    // we cannot select a file here, because no file browser really supports it...
    QFileInfo fi(pathToReveal);

    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absoluteDir().canonicalPath()));
#endif

}

void MainWindow::reveal()
{
    QItemSelectionModel *select = ui->astroListView->selectionModel();
    auto items = select->selectedRows();

    for (auto item: items)
    {
        auto fullPath = sortFilterProxyModel->data(item, AstroFileRoles::FullPathRole).toString();
        revealFile(this, fullPath);
    }
}

void MainWindow::remove()
{
    QItemSelectionModel *select = ui->astroListView->selectionModel();
    auto items = select->selectedRows();

    for (auto item: items)
    {
        auto fullPath = sortFilterProxyModel->data(item, AstroFileRoles::ItemRole).value<AstroFile*>();
        emit removeAstroFileFromCatalog(*fullPath);
    }
}

void MainWindow::createActions()
{
#if defined(Q_OS_WIN)
    QString revealStr = tr("Reveal");
#elif defined(Q_OS_MAC)
    QString revealStr = tr("Reveal");
#else
    QString revealStr = tr("Reveal Folder");
#endif

    revealAct = new QAction(revealStr, this);
    revealAct->setStatusTip(tr("Open the file in the file browser"));
    connect(revealAct, &QAction::triggered, this, &MainWindow::reveal);

    removeAct = new QAction(tr("Remove"), this);
    removeAct->setStatusTip(tr("Removes the image from the catalog. Does not delete the file."));
    connect(removeAct, &QAction::triggered, this, &MainWindow::remove);
}

void MainWindow::duplicatesButton_clicked()
{
    QItemSelectionModel *select = ui->astroListView->selectionModel();
    auto items = select->selectedRows();
    if (items.count() != 1)
        return;

    auto hash = sortFilterProxyModel->data(items[0], AstroFileRoles::FileHashRole).toString();
//    this->sortFilterProxyModel->setDuplicatesFilter(hash);
//    this->sortFilterProxyModel->activateDuplicatesFilter(true);
}

void MainWindow::dbFailedToOpen(const QString message)
{
    QMessageBox msgBox;
    msgBox.setText(tr("Failed to open catalog database"));
    msgBox.setInformativeText(tr("If this error keeps happening, try deleting and re-creating the database. Error Message: ") + message);
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.exec();
    QApplication::quit();
}

void MainWindow::dbAstroFileAdded(const AstroFile &astroFile)
{
    emit catalogAddAstroFile(astroFile);
    numberOfActiveJobs--;
    ui->statusbar->showMessage(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
    importFileDialog.SetQueueSize(numberOfActiveJobs);
}

void MainWindow::importCancelled()
{
    qDebug()<<"Cancelling";
    dbService->cancel();
    emit fileImporter->CancelImport();
}

void MainWindow::importPaused(QAbstractButton* button)
{
    qDebug()<<"Button Clicked";
    QString buttonText = button->text();

    if (buttonText == "Pause")
    {
        button->setText("Resume");
        emit fileImporter->PauseImport();
    }
    else if (buttonText == "Resume")
    {
        button->setText("Pause");
        emit fileImporter->ResumeImport();
    }
}

void MainWindow::filesDropped(const QList<QUrl> &folders)
{
    if (fileImporter != nullptr)
    {
        qDebug()<<"fileImporter already Exists";
        return;
    }
    showImportDialog();
    createFileImporter();
    fileImporter->ImportFiles(folders);
}

void MainWindow::DatabaseQueueLength(int length)
{
    QString labelText;
    if (length > 0)
        labelText = QString("Db Queue: %1").arg(length);
    else
        labelText = "";

    this->dbQueueStatus.setText(labelText);
//    ui->statusbar->showMessage(QString("Db Queue: %1").arg(length));
}

void MainWindow::FileImporterFinished()
{
    if (fileImporter != nullptr)
    {
        delete fileImporter;
        fileImporter = nullptr;
    }
    //    importFileDialog.close();
}

void MainWindow::addedFilterQuery(QString filterKey, QString filterValue)
{
    filters << QPair<QString,QString>(filterKey, filterValue);
    emit loadAstroFilesFromDb(fileExtensionFilter, filters);
    emit loadFileExtensionStats(fileExtensionFilter, filters);
    emit loadFilterStats(fileExtensionFilter, filters);
}

void MainWindow::removedFilterQuery(QString filterKey, QString filterValue)
{
    filters.removeOne(QPair<QString,QString>(filterKey, filterValue));
    if (fileExtensionFilter.isEmpty() && filters.isEmpty())
    {
        emit resetFilters();
    }
    else
    {
        emit loadAstroFilesFromDb(fileExtensionFilter, filters);
    }
    emit loadFileExtensionStats(fileExtensionFilter, filters);
    emit loadFilterStats(fileExtensionFilter, filters);
}

void MainWindow::addedFileExtensionQuery(QString fileExtension)
{
    fileExtensionFilter = fileExtension;
    emit loadAstroFilesFromDb(fileExtensionFilter, filters);
    emit loadFileExtensionStats(fileExtensionFilter, filters);
    emit loadFilterStats(fileExtensionFilter, filters);
}

void MainWindow::removedFileExtensionQuery(QString fileExtension)
{
    fileExtensionFilter.clear();
    if (fileExtensionFilter.isEmpty() && filters.isEmpty())
    {
        emit resetFilters();
    }
    else
    {
        emit loadAstroFilesFromDb(fileExtensionFilter, filters);
    }
    emit loadFileExtensionStats(fileExtensionFilter, filters);
    emit loadFilterStats(fileExtensionFilter, filters);
}
