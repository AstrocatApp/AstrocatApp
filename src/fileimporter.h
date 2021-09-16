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

#ifndef FILEIMPORTER_H
#define FILEIMPORTER_H

#include "catalog.h"
#include "fileprocessfilter.h"
#include "foldercrawler.h"
#include "newfileprocessor.h"

#include <QObject>
#include <QUrl>
#include <QList>
#include <QThread>

class FileImporter : public QObject
{
    Q_OBJECT
public:
    explicit FileImporter(QObject *parent = nullptr);
    ~FileImporter();
    void setCatalog(Catalog* catalog);

signals:
    void ImportStarted();
    void ImportPaused();
    void ImportResumed();
    void ImportCanceled();
    void ImportFinished();

    void CancelCrawler();
    void CancelProcessor();
    void PauseCrawler();
    void PauseProcessor();
    void ResumeCrawler();
    void ResumeProcessor();

    void FilterFile(QFileInfo fileInfo);
    void ProcessFile(QFileInfo fileInfo);

    void StartCrawling(QString path);

    void AstroFileFound(const QFileInfo astroFile);
    void AstroFileImported(const AstroFile& astroFile);
    void AstroFileImporting(const QFileInfo fileInfo);
    void AstroFileIsInCatalog(const QFileInfo fileInfo);

public slots:
    void ImportFiles(const QList<QUrl>& folders);
    void PauseImport();
    void ResumeImport();
    void CancelImport();

    void FilterSaysProcessNewFile(const QFileInfo fileInfo);
    void FilterSaysFileIsModified(const QFileInfo fileInfo);
    void FilterSaysFileIsCurrent(const QFileInfo fileInfo);
    void FilterSaysFileIsRemoved(const QFileInfo fileInfo);
    void CrawlerFoundFile(QFileInfo filePath);
    void CrawlerReportsFinished();
    void FileFilterReportsFinished();
//    void FileFilterReportsModified();
    void NewFileProcessorReportsFinished(const AstroFile& astroFile);

private:
    Catalog* catalog;
    FolderCrawler* crawler;
    QThread* crawlerThread;
    QThread* filterThread;
    QThread* processorThread;
    FileProcessFilter* fileFilter;
    NewFileProcessor* newFileProcessor;
    int numberFoldersCrawled;
    int numberFilesFiltered;
    int numberFilesProcessed;

    void startCrawlerThread();
    void startCrawling(QUrl folder);
    bool isCanceled;
    bool isPaused;
    bool importInProgress;
    bool crawlerInProgress;
    bool fileFilterInProgress;
    bool newFileProcessorInProgress;
    void CheckIfAllFinished();
    void TerminateCrawler();
    void TerminateFileFilter();
    void TerminateNewFileProcessor();
    void Terminate();
    void StartFilter();
    void StartFileProcessor();
};

#endif // FILEIMPORTER_H
