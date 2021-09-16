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

#include "fileimporter.h"
#include "mock_foldercrawler.h"
#include <QDebug>

FileImporter::FileImporter(QObject *parent) : QObject(parent)
{
    catalog = nullptr;
    crawler = nullptr;
    crawlerThread = nullptr;
    processorThread = nullptr;
    fileFilter = nullptr;
    newFileProcessor = nullptr;
    isCanceled = false;
    isPaused = false;
    importInProgress = false;
    importInProgress = false;
    crawlerInProgress = false;
    fileFilterInProgress = false;
    newFileProcessorInProgress = false;
    numberFoldersCrawled = 0;
    numberFilesFiltered = 0;
    numberFilesProcessed = 0;
}

FileImporter::~FileImporter()
{

}

void FileImporter::setCatalog(Catalog *catalog)
{
    this->catalog = catalog;
}

void FileImporter::startCrawlerThread()
{
    crawler = new FolderCrawler();
//    crawler = new Mock_FolderCrawler();
    crawlerThread = new QThread(this);
    crawlerThread->setObjectName("crawlerThread");
    crawler->moveToThread(crawlerThread);

    connect(crawler,       &FolderCrawler::fileFound,              this,                &FileImporter::CrawlerFoundFile);
//    connect(crawler,       &FolderCrawler::fileFound,              this,                &FileImporter::CrawlerFoundFile, Qt::ConnectionType::BlockingQueuedConnection);
    connect(crawler,       &FolderCrawler::endededCrawlingFolder,  this,                &FileImporter::CrawlerReportsFinished);
    connect(this,          &FileImporter::StartCrawling,            crawler,                &FolderCrawler::crawl);
    connect(this,          &FileImporter::CancelCrawler,            crawler,                &FolderCrawler::cancel);
    connect(this,           &FileImporter::PauseCrawler, crawler, &FolderCrawler::pause);
    connect(this,           &FileImporter::ResumeCrawler, crawler, &FolderCrawler::resume);
    connect(crawlerThread,          &QThread::finished,                                 crawler,                &QObject::deleteLater);
    crawlerThread->start();

}

void FileImporter::StartFilter()
{
    fileFilter = new FileProcessFilter();
    fileFilter->setCatalog(catalog);
    filterThread = new QThread(this);
    filterThread->setObjectName("FilterThread");
    fileFilter->moveToThread(filterThread);

//    connect(crawler,       &FolderCrawler::fileFound,              fileFilter,                &FileProcessFilter::filterFile);
    connect(this,    &FileImporter::FilterFile,      fileFilter,               &FileProcessFilter::filterFile);
    connect(fileFilter,    &FileProcessFilter::shouldProcess,      this,               &FileImporter::FilterSaysProcessNewFile);
    connect(fileFilter,    &FileProcessFilter::fileIsCurrent,      this,               &FileImporter::FilterSaysFileIsCurrent);
    connect(fileFilter,    &FileProcessFilter::fileIsModified,     this,               &FileImporter::FilterSaysFileIsModified);
    connect(fileFilter,    &FileProcessFilter::fileIsRemoved,      this,               &FileImporter::FilterSaysFileIsRemoved);
    connect(crawlerThread,          &QThread::finished,                                 fileFilter,                &QObject::deleteLater);
    filterThread->start();
}

void FileImporter::StartFileProcessor()
{
    newFileProcessor = new NewFileProcessor();
    processorThread = new QThread(this);
    newFileProcessor->setCatalog(catalog);
    newFileProcessor->moveToThread(processorThread);
    processorThread->setObjectName("processorThread");
    processorThread->start();

    connect(this,          &FileImporter::ProcessFile,            newFileProcessor,                &NewFileProcessor::processNewFile);
    connect(newFileProcessor,    &NewFileProcessor::astrofileProcessed,      this,               &FileImporter::NewFileProcessorReportsFinished);
    connect(newFileProcessor,    &NewFileProcessor::processingCancelled,      this,               &FileImporter::NewFileProcessorReportsFinished);
    connect(this,          &FileImporter::CancelProcessor,            newFileProcessor,                &NewFileProcessor::cancel);
    connect(this,           &FileImporter::PauseProcessor, newFileProcessor, &NewFileProcessor::pause);
    connect(this,           &FileImporter::ResumeCrawler, newFileProcessor, &NewFileProcessor::resume);
    connect(processorThread,          &QThread::finished,                                 newFileProcessor,                &QObject::deleteLater);
}

void FileImporter::startCrawling(QUrl folder)
{
    crawlerInProgress = true;
    numberFoldersCrawled++;
    emit StartCrawling(folder.path());
}

void FileImporter::ImportFiles(const QList<QUrl> &folders)
{
    qDebug()<<"FileImporter::ImportFiles";
    if (importInProgress)
    {
        qDebug()<<"An import is already in progress";
        return;
    }

    importInProgress = true;

    startCrawlerThread();
    StartFilter();
    StartFileProcessor();

    for (auto& f : folders)
    {
        auto fileName = f.toLocalFile();
        auto fi = QFileInfo(fileName);
        if (fi.isFile())
        {
            numberFilesFiltered++;
            emit FilterFile(fi);
//            emit fileFilter->filterFile(fi);
        }
        else
        {
            startCrawling(f.path());
        }
    }
    emit ImportStarted();
}

void FileImporter::PauseImport()
{
    qDebug()<<"FileImporter::PauseImport";
    isPaused = true;
    if (crawler == nullptr)
        return;
    emit PauseCrawler();
//    fileFilter.pause();
    emit PauseProcessor();

    emit ImportPaused();
}

void FileImporter::ResumeImport()
{
    qDebug()<<"FileImporter::ResumeImport";
    if (crawler == nullptr)
        return;
    isPaused = false;
    emit ResumeCrawler();
//    fileFilter.resume();
    emit ResumeProcessor();

    emit ImportResumed();
}

void FileImporter::TerminateCrawler()
{
    if (crawler == nullptr)
        return;
    crawler->cancel();
    emit CancelCrawler();
    crawlerThread->quit();
    crawlerThread->wait();
    crawlerThread->deleteLater();
//    delete crawlerThread;
//    delete crawler;
    crawlerThread = nullptr;
    crawler = nullptr;
}

void FileImporter::TerminateFileFilter()
{
    if (fileFilter == nullptr)
        return;

    fileFilter->cancel();
    filterThread->quit();
    filterThread->deleteLater();
    filterThread->wait();
//    delete fileFilter;
    fileFilter = nullptr;
}

void FileImporter::TerminateNewFileProcessor()
{
    if (newFileProcessor == nullptr)
        return;

    newFileProcessor->cancel();
//    emit CancelProcessor();
    newFileProcessor->waitForDrain();
    processorThread->quit();
    processorThread->wait();
//    processorThread->deleteLater();
//    delete newFileProcessor;
    newFileProcessor = nullptr;
}

void FileImporter::Terminate()
{
    TerminateNewFileProcessor();
    TerminateFileFilter();
    TerminateCrawler();
    qDebug()<<"Terminated All";
}

void FileImporter::CancelImport()
{
    qDebug()<<"FileImporter::CancelImport";

    isCanceled = true;
    Terminate();
    emit ImportCanceled();
}

void FileImporter::FilterSaysProcessNewFile(const QFileInfo fileInfo)
{
    qDebug()<<"FileImporter::FilterSaysProcessNewFile";
    if (isCanceled)
        return;
    newFileProcessorInProgress = true;
    numberFilesProcessed++;
    emit AstroFileImporting(fileInfo);
//    emit newFileProcessor->processNewFile(fileInfo);
    emit ProcessFile(fileInfo);
    FileFilterReportsFinished();
}

void FileImporter::FilterSaysFileIsModified(const QFileInfo fileInfo)
{
    if (isCanceled)
        return;
    newFileProcessorInProgress = true;
    numberFilesProcessed++;
    emit AstroFileImporting(fileInfo);
//    emit newFileProcessor->processNewFile(fileInfo);
    emit ProcessFile(fileInfo);
    FileFilterReportsFinished();
}

void FileImporter::FilterSaysFileIsCurrent(const QFileInfo fileInfo)
{
    if (isCanceled)
        return;
    AstroFileIsInCatalog(fileInfo);
    FileFilterReportsFinished();
}

void FileImporter::FilterSaysFileIsRemoved(const QFileInfo fileInfo)
{
    if (isCanceled)
        return;
    FileFilterReportsFinished();
}

void FileImporter::CrawlerFoundFile(QFileInfo filePath)
{
    if (isCanceled)
        return;
    fileFilterInProgress = true;
    numberFilesFiltered++;
    qDebug()<<"fileFilter:numberProcessing: " << numberFilesFiltered;
    emit AstroFileFound(filePath);
//    emit fileFilter->filterFile(filePath);
    emit FilterFile(filePath);
}

void FileImporter::CrawlerReportsFinished()
{
    numberFoldersCrawled--;
//    qDebug()<<"folderCrawler:numberProcessing: " << numberFoldersCrawled;
    if (numberFoldersCrawled == 0)
    {
        crawlerInProgress = false;
        CheckIfAllFinished();
    }
}

//void FileImporter::FileFilterReportsModified()
//{
//    numberFilesFiltered--;
//    qDebug()<<"fileFilter:numberProcessing: " << numberFilesFiltered;
//    if (numberFilesFiltered == 0)
//    {
//        fileFilterInProgress = false;
//        CheckIfAllFinished();
//    }
//}

void FileImporter::FileFilterReportsFinished()
{
    if (isCanceled)
        return;
    numberFilesFiltered--;
    qDebug()<<"fileFilter:numberProcessing: " << numberFilesFiltered;
    if (numberFilesFiltered == 0)
    {
        fileFilterInProgress = false;
        CheckIfAllFinished();
    }
}

void FileImporter::NewFileProcessorReportsFinished(const AstroFile& astroFile)
{
    if (isCanceled)
        return;
//    catalog->addAstroFile(astroFile);
    emit AstroFileImported(astroFile);

    numberFilesProcessed--;
    qDebug()<<"newFileProcessor:numberProcessing: " << numberFilesProcessed;
    if (numberFilesProcessed == 0)
    {
        newFileProcessorInProgress = false;
        CheckIfAllFinished();
    }
}

void FileImporter::CheckIfAllFinished()
{
    qDebug()<<"crawlerInProgress: "<<crawlerInProgress;
    qDebug()<<"fileFilterInProgress: "<<fileFilterInProgress;
    qDebug()<<"newFileProcessorInProgress: "<<newFileProcessorInProgress;
    if (!crawlerInProgress && !fileFilterInProgress && !newFileProcessorInProgress)
    {
        qDebug()<<"All workers finished. Terminating FileImporter";
        importInProgress = false;
        Terminate();
        emit ImportFinished();
    }
}
