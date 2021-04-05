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

#include <QContextMenuEvent>
#include <QMessageBox>
#include <QPainter>
#include <QDesktopServices>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      isInitialized(false)
{
    QCoreApplication::setApplicationName("Astrocat");
//    QCoreApplication::setOrganizationName();
    QCoreApplication::setOrganizationDomain("astrocat.app");
    ui->setupUi(this);

    folderCrawlerThread = new QThread(this);
    folderCrawlerWorker = new FolderCrawler;
    folderCrawlerWorker->moveToThread(folderCrawlerThread);

    fileRepositoryThread = new QThread(this);
    fileRepositoryWorker = new FileRepository;
    fileRepositoryWorker->moveToThread(fileRepositoryThread);

    newFileProcessorThread = new QThread(this);
    newFileProcessorWorker = new NewFileProcessor;
    newFileProcessorWorker->moveToThread(newFileProcessorThread);

    fileViewModel = new FileViewModel(ui->astroListView);
    sortFilterProxyModel = new SortFilterProxyModel(ui->astroListView);
    sortFilterProxyModel->setSourceModel(fileViewModel);
    ui->astroListView->setViewMode(QListView::IconMode);
    ui->astroListView->setResizeMode(QListView::Adjust);
    ui->astroListView->setModel(sortFilterProxyModel);
    QItemSelectionModel *selectionModel = ui->astroListView->selectionModel();
    filterView = new FilterView(ui->scrollAreaWidgetContents_2);
    filterView->setModel(sortFilterProxyModel);

    ui->statusbar->addPermanentWidget(&numberOfItemsLabel);
    ui->statusbar->addPermanentWidget(&numberOfVisibleItemsLabel);
    ui->statusbar->addPermanentWidget(&numberOfSelectedItemsLabel);
    //    ui->statusbar->addPermanentWidget(&numberOfActiveJobsLabel);

    ui->astroListView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    createActions();

    connect(this,                   &MainWindow::crawl,                                 folderCrawlerWorker,    &FolderCrawler::crawl);
    connect(this,                   &MainWindow::initializeFileRepository,              fileRepositoryWorker,   &FileRepository::initialize);
    connect(this,                   &MainWindow::deleteAstrofilesInFolder,              fileRepositoryWorker,   &FileRepository::deleteAstrofilesInFolder);
    connect(this,                   &MainWindow::loadModelFromDb,                       fileRepositoryWorker,   &FileRepository::loadModel);
    connect(this,                   &MainWindow::loadModelIntoViewModel,                fileViewModel,          &FileViewModel::setInitialModel);
    connect(this,                   &MainWindow::resetModel,                            fileViewModel,          &FileViewModel::clearModel);
    connect(this,                   &MainWindow::dbAddOrUpdateAstroFile,           fileRepositoryWorker,   &FileRepository::insertAstrofile);
    connect(this,                   &MainWindow::dbAddTags,                             fileRepositoryWorker,   &FileRepository::addTags);
    connect(this,                   &MainWindow::dbAddThumbnail,                        fileRepositoryWorker,   &FileRepository::addThumbnail);
    connect(this,                   &MainWindow::insertAstrofile,                  fileRepositoryWorker,   &FileRepository::insertAstrofile);
    connect(this,                   &MainWindow::processNewFile,                        newFileProcessorWorker, &NewFileProcessor::processNewFile);
    connect(this,                   &MainWindow::dbGetDuplicates,                        fileRepositoryWorker, &FileRepository::getDuplicateFiles);
    connect(folderCrawlerWorker,    &FolderCrawler::fileFound,                          this,                   &MainWindow::newFileFound);
    connect(folderCrawlerThread,    &QThread::finished,                                 folderCrawlerWorker,    &QObject::deleteLater);
    connect(fileRepositoryWorker,   &FileRepository::astroFileDeleted,                  fileViewModel,          &FileViewModel::removeAstroFile);
    connect(fileRepositoryWorker,   &FileRepository::modelLoaded,                       this,                   &MainWindow::modelLoadedFromDb);
    connect(fileRepositoryThread,   &QThread::finished,                                 fileRepositoryWorker,   &QObject::deleteLater);
    connect(newFileProcessorWorker, &NewFileProcessor::astrofileProcessed,              this,                   &MainWindow::astroFileProcessed);
    connect(newFileProcessorWorker, &NewFileProcessor::processingCancelled,             this,                   &MainWindow::processingCancelled);
    connect(newFileProcessorThread, &QThread::finished,                                 newFileProcessorWorker, &QObject::deleteLater);
    connect(&searchFolderDialog,    &SearchFolderDialog::searchFolderAdded,             folderCrawlerWorker,    &FolderCrawler::crawl);
    connect(&searchFolderDialog,    &SearchFolderDialog::searchFolderRemoved,           this,                   &MainWindow::searchFolderRemoved);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::filterMinimumDateChanged,    filterView,             &FilterView::setFilterMinimumDate);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::filterMaximumDateChanged,    filterView,             &FilterView::setFilterMaximumDate);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::filterReset,                 filterView,             &FilterView::searchFilterReset);
    connect(fileViewModel,          &FileViewModel::modelIsEmpty,                       this,                   &MainWindow::setWatermark);
    connect(fileViewModel,          &FileViewModel::itemsAdded,                          this,                   &MainWindow::itemAddedToModel);
    connect(fileViewModel,          &FileViewModel::itemsRemoved,                        this,                   &MainWindow::itemRemovedFromModel);
    connect(fileViewModel,          &FileViewModel::modelReset,                         this,                   &MainWindow::modelReset);
    connect(filterView,             &FilterView::minimumDateChanged,                    sortFilterProxyModel,   &SortFilterProxyModel::setFilterMinimumDate);
    connect(filterView,             &FilterView::maximumDateChanged,                    sortFilterProxyModel,   &SortFilterProxyModel::setFilterMaximumDate);
    connect(filterView,             &FilterView::addAcceptedFilter,                     sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedFilter);
    connect(filterView,             &FilterView::addAcceptedInstrument,                 sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedInstrument);
    connect(filterView,             &FilterView::addAcceptedObject,                     sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedObject);
    connect(filterView,             &FilterView::addAcceptedExtension,                  sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedExtension);
    connect(filterView,             &FilterView::removeAcceptedFilter,                  sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedFilter);
    connect(filterView,             &FilterView::removeAcceptedInstrument,              sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedInstrument);
    connect(filterView,             &FilterView::removeAcceptedObject,                  sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedObject);
    connect(filterView,             &FilterView::removeAcceptedExtension,               sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedExtension);
    connect(filterView,             &FilterView::astroFileAdded,                        this,                   &MainWindow::itemAddedToSortFilterView);
    connect(filterView,             &FilterView::astroFileRemoved,                      this,                   &MainWindow::itemRemovedFromSortFilterView);
    connect(ui->astroListView,      &QWidget::customContextMenuRequested,             this,                   &MainWindow::itemContextMenuRequested);
    connect(selectionModel,         &QItemSelectionModel::selectionChanged,             this,                   &MainWindow::handleSelectionChanged);

    // Enable the tester during development and debugging. Disble before committing
//    tester = new QAbstractItemModelTester(fileViewModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
}

MainWindow::~MainWindow()
{
    cancelPendingOperations();

    qDebug()<<"Cleaning up folderCrawlerThread";
    cleanUpWorker(folderCrawlerThread);

    qDebug()<<"Cleaning up newFileProcessorThread";    
    cleanUpWorker(newFileProcessorThread);

    qDebug()<<"Cleaning up fileRepositoryThread";
    cleanUpWorker(fileRepositoryThread);

    qDebug()<<"Cleaning up fileViewModel";
    delete fileViewModel;
    qDebug()<<"Cleaning up ui";
    delete ui;
    qDebug()<<"Done Cleaning up.";
}

void MainWindow::cancelPendingOperations()
{
    newFileProcessorWorker->cancel();
    fileRepositoryWorker->cancel();
}

void MainWindow::initialize()
{
    if (isInitialized)
        return;

    folderCrawlerThread->start();
    fileRepositoryThread->start();
    newFileProcessorThread->start();

    emit initializeFileRepository();
    _watermarkMessage = "Loading Catalog...";
    setWatermark(true);
    emit loadModelFromDb();
    isInitialized = true;
    emit dbGetDuplicates();

}

void MainWindow::cleanUpWorker(QThread* thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

void MainWindow::newFileFound(const QFileInfo fileInfo)
{
    AstroFile astroFile(fileInfo);

    if (fileViewModel->astroFileExists(fileInfo.absoluteFilePath()))
    {
        // TODO: Only do this if the timestamp is the same.
    }
    else
    {
        numberOfActiveJobs++;
//        this->numberOfActiveJobsLabel.setText(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
        ui->statusbar->showMessage(QString("Jobs Queue: %1").arg(numberOfActiveJobs));

        emit processNewFile(fileInfo);
    }
}

void MainWindow::searchFolderRemoved(const QString folder)
{
    cancelPendingOperations();
    // The source folder was removed by the user. We will need to remove all images in this source folder from the db.
    emit deleteAstrofilesInFolder(folder);
}

void MainWindow::on_imageSizeSlider_valueChanged(int value)
{
    auto currentIndex = ui->astroListView->currentIndex();

    if (!currentIndex.isValid())
        currentIndex = ui->astroListView->indexAt(QPoint(0,0));

    QPersistentModelIndex pIndex(currentIndex);

    // TODO: This call causes filterAcceptsRow() to be called in the sortFilterProxyModel. Investigate and see if we need to fix.
    fileViewModel->setCellSize(value);

    // TODO: This call causes filterAcceptsRow() to be called in the sortFilterProxyModel. Investigate and see if we need to fix.
//    auto scrollToIndex = sortFilterProxyModel->index(currentIndex.row(), currentIndex.column(), QModelIndex());
//    auto scrollToIndex = sortFilterProxyModel->mapFromSource(currentIndex);

    ui->astroListView->scrollTo(pIndex, QAbstractItemView::ScrollHint::EnsureVisible);
//    ui->astroListView->scrollTo(currentIndex, QAbstractItemView::ScrollHint::PositionAtTop);
//    ui->astroListView->scrollTo(currentIndex);
}

QImage MainWindow::makeThumbnail(const QImage &image)
{
    QImage small =image.scaled( QSize(200, 200), Qt::KeepAspectRatio);
    return small;
}

void MainWindow::on_actionFolders_triggered()
{
    searchFolderDialog.exec();
}

void MainWindow::on_actionAbout_triggered()
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

void MainWindow::crawlAllSearchFolders()
{
    QSettings settings;
    auto foldersFromList = settings.value("SearchFolders").value<QList<QString>>();
    for (auto& f : foldersFromList)
    {
        emit crawl(f);
    }
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
    ui->imagesizeLabel->setText(xSize+"x"+ySize);
}

void MainWindow::modelLoadedFromDb(const QList<AstroFile> &files)
{
    _watermarkMessage = DEFAULT_WATERMARK_MESSAGE;
    setWatermark(true);
    emit loadModelIntoViewModel(files);

    crawlAllSearchFolders();
}

void MainWindow::astroFileProcessed(const AstroFile &astroFile)
{
    emit dbAddOrUpdateAstroFile(astroFile);
    emit dbAddTags(astroFile);
    emit dbAddThumbnail(astroFile, astroFile.thumbnail);
    fileViewModel->addAstroFile(astroFile);
    numberOfActiveJobs--;
//    this->numberOfActiveJobsLabel.setText(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
    ui->statusbar->showMessage(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
}

void MainWindow::processingCancelled(const QFileInfo &fileInfo)
{
    numberOfActiveJobs--;
    ui->statusbar->showMessage(QString("Jobs Queue: %1").arg(numberOfActiveJobs));
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

        QPalette palette;
        palette.setBrush(QPalette::Base, brush);
        ui->astroListView->setPalette(palette);
    }
    else
    {
        ui->astroListView->setPalette(QPalette());
    }
}

void MainWindow::itemAddedToModel(int numberAdded)
{
    this->numberOfItems += numberAdded;
    this->numberOfItemsLabel.setText(QString("Items: %1").arg(numberOfItems));
}

void MainWindow::itemRemovedFromModel(int numberRemoved)
{
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

    QItemSelectionModel *select = ui->astroListView->selectionModel();
    auto items = select->selectedRows();

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

    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    const QString explorer = Environment::systemEnvironment().searchInPath(QLatin1String("explorer.exe"));
    if (explorer.isEmpty()) {
        QMessageBox::warning(parent,
                             tr("Launching Windows Explorer failed"),
                             tr("Could not find explorer.exe in path to launch Windows Explorer."));
        return;
    }
    QString param;
    if (!QFileInfo(pathIn).isDir())
        param = QLatin1String("/select,");
    param += QDir::toNativeSeparators(pathIn);
    QString command = explorer + " " + param;
    QString command = explorer + " " + param;
    QProcess::startDetached(command);

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
    const QFileInfo fileInfo(pathIn);
    const QString folder = fileInfo.absoluteFilePath();
    const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
    QProcess browserProc;
    const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
    bool success = browserProc.startDetached(browserArgs);
    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
    success = success && error.isEmpty();
    if (!success)
        showGraphicalShellError(parent, app, error);
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
        auto fullPath = sortFilterProxyModel->data(item, AstroFileRoles::FullPathRole).toString();
        // TODO: Remove it here
    }
}

void MainWindow::createActions()
{
    revealAct = new QAction(tr("Reveal"), this);
    revealAct->setStatusTip(tr("Open the file in the file browser"));
    connect(revealAct, &QAction::triggered, this, &MainWindow::reveal);

    removeAct = new QAction(tr("Remove"), this);
    removeAct->setStatusTip(tr("Removes the image from the catalog. Does not delete the file."));
    connect(removeAct, &QAction::triggered, this, &MainWindow::remove);
}

void MainWindow::on_duplicatesButton_clicked()
{
    QItemSelectionModel *select = ui->astroListView->selectionModel();
    auto items = select->selectedRows();
    if (items.count() != 1)
        return;

    auto hash = sortFilterProxyModel->data(items[0], AstroFileRoles::FileHashRole).toString();
    this->sortFilterProxyModel->setDuplicatesFilter(hash);
    this->sortFilterProxyModel->activateDuplicatesFilter(true);
}
