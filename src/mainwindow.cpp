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

#include <QMessageBox>
#include <QPainter>

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
    filterWidget = new FilterWidget(ui->scrollAreaWidgetContents_2);

    connect(this,                   &MainWindow::crawl,                                 folderCrawlerWorker,    &FolderCrawler::crawl);
    connect(this,                   &MainWindow::initializeFileRepository,              fileRepositoryWorker,   &FileRepository::initialize);
    connect(this,                   &MainWindow::deleteAstrofilesInFolder,              fileRepositoryWorker,   &FileRepository::deleteAstrofilesInFolder);
    connect(this,                   &MainWindow::loadModelFromDb,                       fileRepositoryWorker,   &FileRepository::loadModel);
    connect(this,                   &MainWindow::loadModelIntoViewModel,                fileViewModel,          &FileViewModel::setInitialModel);
    connect(this,                   &MainWindow::resetModel,                            fileViewModel,          &FileViewModel::clearModel);
    connect(this,                   &MainWindow::itemModelAddTags,                      fileViewModel,          &FileViewModel::addTags);
    connect(this,                   &MainWindow::itemModelAddThumbnail,                 fileViewModel,          &FileViewModel::addThumbnail);
    connect(this,                   &MainWindow::dbAddTags,                             fileRepositoryWorker,   &FileRepository::addTags);
    connect(this,                   &MainWindow::dbAddThumbnail,                        fileRepositoryWorker,   &FileRepository::addThumbnail);
    connect(this,                   &MainWindow::insertAstrofileImage,                  fileRepositoryWorker,   &FileRepository::insertAstrofileImage);
    connect(this,                   &MainWindow::extractTags,                           newFileProcessorWorker, &NewFileProcessor::extractTags);
    connect(this,                   &MainWindow::extractThumbnail,                      newFileProcessorWorker, &NewFileProcessor::extractThumbnail);
    connect(this,                   &MainWindow::processNewFile,                        newFileProcessorWorker, &NewFileProcessor::processNewFile);
    connect(folderCrawlerWorker,    &FolderCrawler::fileFound,                          this,                   &MainWindow::newFileFound);
    connect(folderCrawlerThread,    &QThread::finished,                                 folderCrawlerWorker,    &QObject::deleteLater);
    connect(fileRepositoryWorker,   &FileRepository::getTagsFinished,                   filterWidget,           &FilterWidget::setAllTags);
    connect(fileRepositoryWorker,   &FileRepository::astroFileDeleted,                  fileViewModel,          &FileViewModel::removeAstroFile);
    connect(fileRepositoryWorker,   &FileRepository::modelLoaded,                       this,                   &MainWindow::modelLoadedFromDb);
    connect(fileRepositoryThread,   &QThread::finished,                                 fileRepositoryWorker,   &QObject::deleteLater);
    connect(newFileProcessorWorker, &NewFileProcessor::thumbnailExtracted,              this,                   &MainWindow::thumbnailExtracted);
    connect(newFileProcessorWorker, &NewFileProcessor::tagsExtracted,                   this,                   &MainWindow::tagsExtracted);
    connect(newFileProcessorThread, &QThread::finished,                                 newFileProcessorWorker, &QObject::deleteLater);
    connect(&searchFolderDialog,    &SearchFolderDialog::searchFolderAdded,             folderCrawlerWorker,    &FolderCrawler::crawl);
    connect(&searchFolderDialog,    &SearchFolderDialog::searchFolderRemoved,           this,                   &MainWindow::searchFolderRemoved);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::filterMinimumDateChanged,    filterWidget,           &FilterWidget::setFilterMinimumDate);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::filterMaximumDateChanged,    filterWidget,           &FilterWidget::setFilterMaximumDate);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::filterReset,                 filterWidget,           &FilterWidget::searchFilterReset);
    connect(sortFilterProxyModel,   &SortFilterProxyModel::astroFileAccepted,           filterWidget,           &FilterWidget::addAstroFileTags);
    connect(fileViewModel,          &FileViewModel::modelIsEmpty,                       this,                   &MainWindow::setWatermark);
    connect(filterWidget,           &FilterWidget::minimumDateChanged,                  sortFilterProxyModel,   &SortFilterProxyModel::setFilterMinimumDate);
    connect(filterWidget,           &FilterWidget::maximumDateChanged,                  sortFilterProxyModel,   &SortFilterProxyModel::setFilterMaximumDate);
    connect(filterWidget,           &FilterWidget::addAcceptedFilter,                   sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedFilter);
    connect(filterWidget,           &FilterWidget::addAcceptedInstrument,               sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedInstrument);
    connect(filterWidget,           &FilterWidget::addAcceptedObject,                   sortFilterProxyModel,   &SortFilterProxyModel::addAcceptedObject);
    connect(filterWidget,           &FilterWidget::removeAcceptedFilter,                sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedFilter);
    connect(filterWidget,           &FilterWidget::removeAcceptedInstrument,            sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedInstrument);
    connect(filterWidget,           &FilterWidget::removeAcceptedObject,                sortFilterProxyModel,   &SortFilterProxyModel::removeAcceptedObject);
    connect(selectionModel,         &QItemSelectionModel::selectionChanged,             this,                   &MainWindow::handleSelectionChanged);

    // Enable the tester during development and debugging. Disble before committing
//    tester = new QAbstractItemModelTester(fileViewModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
}

MainWindow::~MainWindow()
{
    qDebug()<<"Cleaning up folderCrawlerThread";
    cleanUpWorker(folderCrawlerThread);

    qDebug()<<"Cleaning up newFileProcessorThread";
    newFileProcessorWorker->cancel();
    cleanUpWorker(newFileProcessorThread);

    qDebug()<<"Cleaning up fileRepositoryThread";
    fileRepositoryWorker->cancel();
    cleanUpWorker(fileRepositoryThread);

    qDebug()<<"Cleaning up fileViewModel";
    delete fileViewModel;
    qDebug()<<"Cleaning up ui";
    delete ui;
    qDebug()<<"Done Cleaning up.";
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
    AstroFileImage afi(astroFile, QImage());

    if (fileViewModel->astroFileExists(fileInfo.absoluteFilePath()))
    {
        // TODO: Only do this if the timestamp is the same.
        qDebug() << "File already in model";
    }
    else
    {
        fileViewModel->addAstroFile(afi);
        emit insertAstrofileImage(afi);
        emit processNewFile(fileInfo);
    }
}

void MainWindow::searchFolderRemoved(const QString folder)
{
    // The source folder was removed by the user. We will need to remove all images in this source folder from the db.
    emit deleteAstrofilesInFolder(folder);
}

void MainWindow::on_imageSizeSlider_valueChanged(int value)
{
    auto currentIndex = ui->astroListView->currentIndex();

    if (!currentIndex.isValid())
        currentIndex = ui->astroListView->indexAt(QPoint(0,0));

    fileViewModel->setCellSize(value);

    auto scrollToIndex = sortFilterProxyModel->index(currentIndex.row(), currentIndex.column(), QModelIndex());

    ui->astroListView->scrollTo(scrollToIndex, QAbstractItemView::ScrollHint::PositionAtTop);
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

void MainWindow::modelLoadedFromDb(const QList<AstroFileImage> &files)
{
    _watermarkMessage = DEFAULT_WATERMARK_MESSAGE;
    setWatermark(true);
    emit loadModelIntoViewModel(files);
    for (auto& i : files)
    {
        if (i.thumbnailStatus == NotProcessedYet)
            emit extractThumbnail(i);
        if (i.tagStatus == TagNotProcessedYet)
            emit extractTags(i);
    }

    crawlAllSearchFolders();
}

void MainWindow::tagsExtracted(const AstroFileImage &astroFileImage, const QMap<QString, QString> &tags)
{
    AstroFileImage afi(astroFileImage);
    afi.tagStatus = TagExtracted;
    afi.astroFile.Tags.clear();
    afi.astroFile.Tags.insert(tags);

    emit dbAddTags(afi);
    emit itemModelAddTags(afi);
}

void MainWindow::thumbnailExtracted(const AstroFileImage &astroFileImage, const QImage &img)
{
    AstroFileImage afi(astroFileImage);
    afi.thumbnailStatus = Loaded;
    afi.image = img;

    emit dbAddThumbnail(afi, img);
    emit itemModelAddThumbnail(afi);
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
