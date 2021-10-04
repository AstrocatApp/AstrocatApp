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

#include "dbservice.h"

#include <QFuture>
#include <QThread>
#include <QtConcurrent>

DbService::DbService(QObject *parent) : QObject(parent)
{
    cancelSignaled = false;
    fileRepository = new FileRepository("BackgroundThreadConnection");
    fileRepositoryThread = new QThread(this);
    fileRepository->moveToThread(fileRepositoryThread);
    fileRepositoryThread->start();

//    fileRepositoryOnUiThread = new FileRepository("UiThreadConnection");

    connect(this, &DbService::FileRepository_addAstrofile, fileRepository, &FileRepository::addAstrofile, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_deleteAstrofile, fileRepository, &FileRepository::deleteAstrofile, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_deleteAstrofilesInFolder, fileRepository, &FileRepository::deleteAstrofilesInFolder, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_initialize, fileRepository, &FileRepository::initialize, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_loadModel, fileRepository, &FileRepository::loadModel, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_getDuplicateFiles, fileRepository, &FileRepository::getDuplicateFiles, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_getDuplicateFilesByFileHash, fileRepository, &FileRepository::getDuplicateFilesByFileHash, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_getDuplicateFilesByImageHash, fileRepository, &FileRepository::getDuplicateFilesByImageHash, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_loadThumbnail, fileRepository, &FileRepository::loadThumbnail, Qt::BlockingQueuedConnection);
    connect(this, &DbService::FileRepository_loadFilterStats, fileRepository, &FileRepository::loadTagStats);
    connect(this, &DbService::FileRepository_loadFileExtensionStats, fileRepository, &FileRepository::loadFileExtensionStats);
    connect(this, &DbService::FileRepository_loadAstroFiles, fileRepository, &FileRepository::loadAstroFiles);

    connect(fileRepository, &FileRepository::getAllAstroFilesFinished,this, &DbService::getAllAstroFilesFinished);
    connect(fileRepository, &FileRepository::getTagsFinished, this, &DbService::getTagsFinished);
    connect(fileRepository, &FileRepository::astroFileDeleted, this, &DbService::astroFileDeleted);
    connect(fileRepository, &FileRepository::astroFilesDeleted, this, &DbService::astroFilesDeleted);
    connect(fileRepository, &FileRepository::modelLoaded, this, &DbService::modelLoaded);
    connect(fileRepository, &FileRepository::dbFailedToInitialize, this, &DbService::dbFailedToInitialize);
    connect(fileRepository, &FileRepository::astroFileAdded, this, &DbService::astroFileAdded);
    connect(fileRepository, &FileRepository::thumbnailLoaded, this, &DbService::thumbnailLoaded);
    connect(fileRepository, &FileRepository::modelLoadingGotAstrofiles, this, &DbService::modelLoadingGotAstrofiles);
    connect(fileRepository, &FileRepository::modelLoadingGotTags, this, &DbService::modelLoadingGotTags);
    connect(fileRepository, &FileRepository::modelLoadingGotThumbnails, this, &DbService::modelLoadingGotThumbnails);
    connect(fileRepository, &FileRepository::astroFilesInFilter, this, &DbService::astroFilesInFilter);

//    connect(this, &DbService::FilterStatsLoaded, fileRepository, &FileRepository::filterStatsLoaded);
//    connect(this, &DbService::FileExtensionStatsLoaded, fileRepository, &FileRepository::fileExtensionStatsLoaded);

    connect(fileRepository, &FileRepository::filterStatsLoaded, this, &DbService::FilterStatsLoaded);
    connect(fileRepository, &FileRepository::fileExtensionStatsLoaded, this, &DbService::FileExtensionStatsLoaded);

    QFuture<void> future = QtConcurrent::run(&threadPool, [=]()
    {
        while(true)
        {
            opsListMutex.lock();
            if (opsQueue.isEmpty())
            {
                emit DatabaseQueueLength(0);
                waitCondition.wait(&opsListMutex);
            }
            opsListMutex.unlock();
            if (cancelSignaled)
            {
                qDebug()<<"Cancel signaled";
                opsQueue.clear();
                int length = opsQueue.length();
                qDebug()<<"Ops Queue size:" << length;
                emit DatabaseQueueLength(length);
                cancelSignaled = false;
                return;
            }
            opsListMutex.lock();
            auto length = opsQueue.length();
            auto ops = opsQueue.takeFirst();
            opsListMutex.unlock();
            emit DatabaseQueueLength(length);
            processOps(ops);
        }
    }
    );
}

DbService::~DbService()
{
    fileRepository->cancel();
    fileRepositoryThread->quit();
    fileRepositoryThread->wait();
    delete fileRepository;
    fileRepository = nullptr;
    delete fileRepositoryThread;
    fileRepositoryThread = nullptr;

    delete fileRepositoryOnUiThread;
    fileRepositoryOnUiThread = nullptr;
}

void DbService::cancel()
{
    // TODO: the cancel operation should not cancel all.
    // We should only cancel a certain import, otherwise pending edits
    // will also get cancelled.
    // Pass an identifier to cancel and filter out only requests
    // with that identifier.
    cancelSignaled = true;
    opsQueue.append(OpsNode(OpsOperation::cancel, AstroFile(), QString()));
}

void DbService::deleteAstrofile(const AstroFile &afi)
{
    OpsNode ops = OpsNode(OpsOperation::deleteAsrofile, afi, QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::deleteAstrofilesInFolder(const QString &fullPath)
{
    OpsNode ops = OpsNode(OpsOperation::deleteAstrofilesInFolder, AstroFile(), fullPath);
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::initialize()
{
    OpsNode ops = OpsNode(OpsOperation::initialize, AstroFile(), QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::loadModel()
{
    OpsNode ops = OpsNode(OpsOperation::loadModel, AstroFile(), QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::addAstrofile(const AstroFile &afi)
{
    OpsNode ops = OpsNode(OpsOperation::addAstrofile, afi, QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::getDuplicateFiles()
{
    OpsNode ops = OpsNode(OpsOperation::getDuplicateFiles, AstroFile(), QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::getDuplicateFilesByFileHash()
{
    OpsNode ops = OpsNode(OpsOperation::getDuplicateFilesByFileHash, AstroFile(), QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::getDuplicateFilesByImageHash()
{
    OpsNode ops = OpsNode(OpsOperation::getDuplicateFilesByImageHash, AstroFile(), QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::loadThumbnail(const AstroFile &afi)
{
    OpsNode ops =  OpsNode(OpsOperation::loadThumbnail, afi, QString(""));
    opsListMutex.lock();
    opsQueue.append(ops);
    opsListMutex.unlock();
    waitCondition.wakeAll();
}

void DbService::loadFilterStats(const QString fileExtension, QList<QPair<QString, QString>>& filters)
{
    emit FileRepository_loadFilterStats(fileExtension, filters);
}

void DbService::loadFileExtensionStats(const QString fileExtension, QList<QPair<QString, QString>>& filters)
{
    emit FileRepository_loadFileExtensionStats(fileExtension, filters);
}

void DbService::loadAstroFiles(const QString fileExtension, QList<QPair<QString, QString> > &filters)
{
    emit FileRepository_loadAstroFiles(fileExtension, filters);
}

void DbService::processOps(const OpsNode& ops)
{
    switch (ops.operation)
    {
        case OpsOperation::deleteAsrofile:
        {
            auto& p = ops.astroFile;
            emit FileRepository_deleteAstrofile(p);
            break;
        }
        case OpsOperation::deleteAstrofilesInFolder:
        {
            auto &p = ops.path;
            emit FileRepository_deleteAstrofilesInFolder(p);
            break;
        }
        case OpsOperation::initialize:
        {
            emit FileRepository_initialize();
            break;
        }
        case OpsOperation::loadModel:
        {
            emit FileRepository_loadModel();
            break;
        }
        case OpsOperation::addAstrofile:
        {
            auto& p = ops.astroFile;
            emit FileRepository_addAstrofile(p);
            break;
        }
        case OpsOperation::getDuplicateFiles:
        {
            emit FileRepository_getDuplicateFiles();
            break;
        }
        case OpsOperation::getDuplicateFilesByImageHash:
        {
            emit FileRepository_getDuplicateFilesByImageHash();
            break;
        }
        case OpsOperation::getDuplicateFilesByFileHash:
        {
            emit FileRepository_getDuplicateFilesByFileHash();
            break;
        }
        case OpsOperation::loadThumbnail:
        {
            auto& p = ops.astroFile;
            emit FileRepository_loadThumbnail(p);
            break;
        }
        case OpsOperation::cancel:
        {
            qDebug()<<"Cancel not implemented";
            Q_ASSERT(false);
        }
    }
}
