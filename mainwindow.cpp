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
#include "searchfolderdialog.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set up Folder Crawler Worker
    folderCrawlerThread = new QThread(this);
    folderCrawlerWorker = new FolderCrawler;
    folderCrawlerWorker->moveToThread(folderCrawlerThread);
    connect(this, &MainWindow::Crawl, folderCrawlerWorker, &FolderCrawler::Crawl);
    connect(folderCrawlerWorker, &FolderCrawler::FileFound, this, &MainWindow::NewFileFound);
    connect(folderCrawlerThread, &QThread::finished, folderCrawlerWorker, &QObject::deleteLater);
    folderCrawlerThread->start();
    // End - Set up Folder Crawler Worker

    // Set up File Repository Worker
    fileRepositoryThread = new QThread(this);
    fileRepositoryWorker = new FileRepository;
    fileRepositoryWorker->moveToThread(fileRepositoryThread);
    connect(this, &MainWindow::GetAstroFile, fileRepositoryWorker, &FileRepository::getAstrofile);
    connect(this, &MainWindow::GetAllAstroFiles, fileRepositoryWorker, &FileRepository::getAllAstrofiles);
    connect(this, &MainWindow::InsertAstroFile, fileRepositoryWorker, &FileRepository::insertAstrofile);
    connect(this, &MainWindow::DbAddTags, fileRepositoryWorker, &FileRepository::AddTags);
    connect(this, &MainWindow::DbAddThumbnail, fileRepositoryWorker, &FileRepository::AddThumbnail);
    connect(this, &MainWindow::DbGetThumbnails, fileRepositoryWorker, &FileRepository::GetThumbnails);
    connect(this, &MainWindow::InitializeFileRepository, fileRepositoryWorker, &FileRepository::Initialize);
    connect(this, &MainWindow::GetAllAstroFileTags, fileRepositoryWorker, &FileRepository::GetTags);
    connect(this, &MainWindow::DeleteAstrofilesInFolder, fileRepositoryWorker, &FileRepository::deleteAstrofilesInFolder);
    connect(fileRepositoryWorker, &FileRepository::getAstroFileFinished, this, &MainWindow::GetAstroFileFinished);
    connect(fileRepositoryWorker, &FileRepository::getAllAstroFilesFinished, this, &MainWindow::GetAllAstroFilesFinished);
    connect(fileRepositoryWorker, &FileRepository::getThumbnailFinished, this, &MainWindow::GetThumbnailFinished);
    connect(fileRepositoryWorker, &FileRepository::getTagsFinished, this, &MainWindow::GetAllAstroFileTagsFinished);
    connect(fileRepositoryThread, &QThread::finished, fileRepositoryWorker, &QObject::deleteLater);
    fileRepositoryThread->start();
    // End - Set up File Repository Worker

    // Set up Fits Processor Worker
    fitsProcessorThread = new QThread(this);
    fitsProcessorWorker = new FitsProcessor;
    fitsProcessorWorker->moveToThread(fitsProcessorThread);
    connect(this, &MainWindow::ProcessFitsFile, fitsProcessorWorker, &FitsProcessor::ProcessFitsFile);
    connect(fitsProcessorWorker, &FitsProcessor::ProcessFitsFileFinished, this, &MainWindow::ProcessFitsFileFinished);
    connect(fitsProcessorThread, &QThread::finished, fitsProcessorWorker, &QObject::deleteLater);
    fitsProcessorThread->start();
    // End - Set up Fits Processor Worker

    // Set up Fileview Model
    fileViewModel = new FileViewModel(ui->astroListView);
    connect(fileViewModel, &FileViewModel::GetThumbnail, fileRepositoryWorker, &FileRepository::GetThumbnail);
    connect(fileRepositoryWorker, &FileRepository::getThumbnailFinished, fileViewModel, &FileViewModel::GetThumbnailFinished);
    connect(fileRepositoryWorker, &FileRepository::astroFileDeleted, fileViewModel, &FileViewModel::RemoveAstroFile);
    ui->astroListView->setViewMode(QListView::IconMode);
    ui->astroListView->setIconSize(QSize(100,100));
    ui->astroListView->setResizeMode(QListView::Adjust);
    ui->astroListView->setModel(fileViewModel);
    // End - Set up Fileview Model

    // Set up SearchFolderDialog
    connect(&searchFolderDialog, &SearchFolderDialog::SearchFolderAdded, folderCrawlerWorker, &FolderCrawler::Crawl);
    connect(&searchFolderDialog, &SearchFolderDialog::SearchFolderRemoved, this, &MainWindow::SearchFolderRemoved);
    // End - Set up SearchFolderDialog

    emit InitializeFileRepository();

    emit GetAllAstroFiles();
}

void CleanUpWorker(QThread* thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

MainWindow::~MainWindow()
{
    qDebug()<<"Cleaning up folderCrawlerThread";
    CleanUpWorker(folderCrawlerThread);

    qDebug()<<"Cleaning up fitsProcessorThread";
    fitsProcessorWorker->Cancel();
    CleanUpWorker(fitsProcessorThread);

    qDebug()<<"Cleaning up fileRepositoryThread";
    CleanUpWorker(fileRepositoryThread);

    qDebug()<<"Cleaning up fileViewModel";
    delete fileViewModel;
    qDebug()<<"Cleaning up ui";
    delete ui;
    qDebug()<<"Done Cleaning up.";
}

void MainWindow::NewFileFound(const QFileInfo fileInfo)
{
    AstroFile astroFile;
    astroFile.FullPath = fileInfo.absoluteFilePath();
    astroFile.CreatedTime = fileInfo.birthTime();
    astroFile.LastModifiedTime = fileInfo.lastModified();
    astroFile.DirectoryPath = fileInfo.canonicalPath();
    astroFile.FileType = fileInfo.suffix();
    astroFile.FileName = fileInfo.baseName();

    if (fileViewModel->AstroFileExists(fileInfo.absoluteFilePath()))
    {
        qDebug() << "File already in DB";
    }
    else
    {
        QImage img;
        fileViewModel->AddAstroFile(astroFile, img);
        emit InsertAstroFile(astroFile);
        emit ProcessFitsFile(astroFile);
        emit GetAstroFile(fileInfo.absoluteFilePath());
    }
}

void MainWindow::GetAstroFileFinished(const AstroFile astroFile)
{
//    qDebug() << "AstroFile from DB: " << astroFile.FullPath;

}

void MainWindow::ProcessFitsFileFinished(const AstroFile astroFile, const QImage& img, long nX, long nY )
{
    emit DbAddTags(astroFile);
    QImage thumbnail = MakeThumbnail(img);
    emit DbAddThumbnail(astroFile, thumbnail);

    fileViewModel->AddAstroFile(astroFile, img.scaled( 400, 400, Qt::KeepAspectRatio));
}

void MainWindow::SearchFolderRemoved(const QString folder)
{
    // The source folder was removed by the user. We will need to remove all images in this source folder fromthe db.
    emit DeleteAstrofilesInFolder(folder);
}

void MainWindow::on_pushButton_clicked()
{
    searchFolderDialog.exec();
}

void MainWindow::on_imageSizeSlider_valueChanged(int value)
{
    emit fileViewModel->setCellSize(value);
}

void MainWindow::GetAllAstroFilesFinished(const QList<AstroFile> & files)
{
    fileViewModel->SetInitialAstrofiles(files);
}

void MainWindow::GetAllAstroFileTagsFinished(const QMap<QString, QSet<QString>> & tags)
{
    qDebug()<< "Got Tags";
    QMapIterator<QString, QSet<QString>> iter(tags);

    while(iter.hasNext())
    {
        iter.next();
        qDebug() << "===" << iter.key();
        QSetIterator setiter(iter.value());
        while (setiter.hasNext())
        {
            qDebug() << "------" << setiter.next();
        }
    }

}

void MainWindow::GetThumbnailFinished(const AstroFile& astroFile, const QPixmap& pixmap)
{
}

QImage MainWindow::MakeThumbnail(const QImage &image)
{
    QImage small =image.scaled( QSize(200, 200), Qt::KeepAspectRatio);
    return small;
}
