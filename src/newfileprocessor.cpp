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

#include "fileprocessor.h"
#include "imageprocessor.h"
#include "xisfprocessor.h"
#include "newfileprocessor.h"
#include "fitsprocessor.h"

#include <QCryptographicHash>
#include <QtConcurrent>

NewFileProcessor::NewFileProcessor(QObject *parent) : QObject(parent)
{
    catalog = nullptr;
    threadPool.setObjectName("NewFileProcessor");
}

void NewFileProcessor::setCatalog(Catalog *cat)
{
    this->catalog = cat;
}

void NewFileProcessor::processNewFile(const QFileInfo& fileInfo)
{
//    qDebug()<<"Processing new file: " << fileInfo.path();
    Q_ASSERT(catalog != nullptr);

    if (cancelSignaled)
    {
        emit processingCancelled(fileInfo);
        return;
    }
    pauseMutex.lock();
    if (pauseSignaled)
    {
        pauseCondition.wait(&pauseMutex);
    }
    pauseMutex.unlock();

    QStorageInfo storageInfo = QStorageInfo(fileInfo.canonicalFilePath());

    QFuture<void> future = QtConcurrent::run(&threadPool, [=]() {
        if (cancelSignaled)
        {
            emit processingCancelled(fileInfo);
            return;
        }
        pauseMutex.lock();
        if (pauseSignaled)
        {
            pauseCondition.wait(&pauseMutex);
        }
        pauseMutex.unlock();
        AstroFile astroFile(fileInfo);
        astroFile.VolumeName = storageInfo.name();
        astroFile.VolumeRoot = storageInfo.rootPath();

        if (catalog->shouldProcessFile(fileInfo) == RemovedFile)
        {
            // This file is not in the catalog anymore.
            emit processingCancelled(fileInfo);
            return;
        }

        FileProcessor* processor = getProcessorForFile(astroFile);
        if (processor == nullptr)
        {
            emit processingCancelled(fileInfo);
            return;
        }

        if (!processor->loadFile(astroFile))
        {
            // This is an invalid file.
            astroFile.processStatus = AstroFileFailedToProcess;
            emit astrofileProcessed(astroFile);
            delete processor;
            return;
        }
//        if (cancelSignaled)
//        {
//            delete processor;
//            return;
//        }
        processor->extractTags();

        auto tags = processor->getTags();
        astroFile.Tags.swap(tags);
        astroFile.tagStatus = TagExtracted;

//        if (cancelSignaled)
//        {
//            delete processor;
//            return;
//        }
        processor->extractThumbnail();
        auto thumbnail = processor->getThumbnail();
        astroFile.thumbnail = thumbnail;
        astroFile.tinyThumbnail = processor->getTinyThumbnail();
        astroFile.thumbnailStatus = ThumbnailLoaded;

//        if (cancelSignaled)
//        {
//            delete processor;
//            return;
//        }
        QString hash = getFileHash(fileInfo).toHex();
        astroFile.FileHash = hash;

//        if (cancelSignaled)
//        {
//            delete processor;
//            return;
//        }
        QString imageHash = processor->getImageHash().toHex();
        astroFile.ImageHash = imageHash;
        delete processor;
        astroFile.processStatus = AstroFileProcessed;

        emit astrofileProcessed(astroFile);
    });
    futures.append(futures);
}

QByteArray fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile sourceFile(fileName);
    qint64 fileSize = sourceFile.size();
    const qint64 bufferSize = 10240;

    if (sourceFile.open(QIODevice::ReadOnly))
    {
        char buffer[bufferSize];
        int bytesRead;
        int readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash(hashAlgorithm);
        while (readSize > 0 && (bytesRead = sourceFile.read(buffer, readSize)) > 0)
        {
            fileSize -= bytesRead;
            hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);
        }

        sourceFile.close();
        return hash.result();
    }
    return QByteArray();
}

QByteArray NewFileProcessor::getFileHash(const QFileInfo &fileInfo)
{
    return fileChecksum(fileInfo.absoluteFilePath(), QCryptographicHash::Sha1);
}

void NewFileProcessor::cancel()
{
    cancelSignaled = true;
}

FileProcessor* NewFileProcessor::getProcessorForFile(const QFileInfo &fileInfo)
{
    const AstroFile af(fileInfo);
    return getProcessorForFile(af);
}

FileProcessor* NewFileProcessor::getProcessorForFile(const AstroFile &astroFile)
{
    switch (astroFile.FileType)
    {
        case AstroFileType::Fits:
            return new FitsProcessor();
        case AstroFileType::Xisf:
            return new XisfProcessor();
        case AstroFileType::Image:
            return new ImageProcessor();
        default: return nullptr;
    }
}

void NewFileProcessor::pause()
{
    pauseMutex.lock();
    pauseSignaled = true;
    pauseMutex.unlock();
}

void NewFileProcessor::resume()
{
    pauseMutex.lock();
    pauseSignaled = false;
    pauseMutex.unlock();
    pauseCondition.wakeAll();
}

void NewFileProcessor::waitForDrain()
{
    for(auto future:futures)
    {
        future.waitForFinished();
    }
}

