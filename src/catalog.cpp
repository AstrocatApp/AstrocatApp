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

#include "catalog.h"

#include <QTimer>


// We should check if the folder is a child folder of an
// existing search folder.
// Or if we add a parent folder, we might want to remove
// child folders
Catalog::Catalog(QObject *parent)
{
//    QTimer *timer = new QTimer(this);
    connect(&timer, &QTimer::timeout, this, QOverload<>::of(&Catalog::pushProcessedQueue));
    astroFilesQueue = 0;
    timer.start(500);
}

Catalog::~Catalog()
{

}

void Catalog::cancel()
{
    cancelSignaled = true;
    timer.stop();
}

void Catalog::addSearchFolder(const QString &folder)
{
    QMutexLocker locker(&searchFoldersMutex);
    searchFolders.append(folder);
}

void Catalog::addSearchFolder(const QList<QString> &folders)
{
    QMutexLocker locker(&searchFoldersMutex);
    searchFolders.append(folders);
}

void Catalog::removeSearchFolder(const QString &folder)
{
    QMutexLocker locker(&searchFoldersMutex);
    searchFolders.removeOne(folder);
}

void Catalog::removeAllSearchFolders()
{
    QMutexLocker locker(&searchFoldersMutex);
    searchFolders.clear();
}

void Catalog::impAddAstroFile(const AstroFile &astroFile, bool shouldEmit)
{
    QMutexLocker locker(&listMutex);

    // Check if this file already exists
    // We should probably check by id, but we don't have that mapping
    // Getting the index is O(n), and it may take a long time to
    // call it on initialization of large DBs.
    // i.e. don't call `int index = astroFileIndex(astroFile);` here

    auto existing = getAstroFileByPath(astroFile.FullPath);
    if (existing == nullptr)
    {
        AstroFile* a = new AstroFile(astroFile);
        astroFiles.append(a);
        filePathToIdMap.insert(astroFile.FullPath, a);
        if (shouldEmit)
        {
//            emit AstroFilesAdded(1);
            astroFilesQueueMutex.lock();
            astroFilesQueue++;
            astroFilesQueueMutex.unlock();

        }
    }
    else
    {
        int index = astroFileIndex(astroFile);
        if (index == -1)
        {
            qDebug()<<"=== BUG: Found two files with same path"<<astroFile.FullPath;
            qDebug()<<"File1: " << existing->Id;
            qDebug()<<"File2: " << astroFile.Id;
            return;
        }
        AstroFile* a = new AstroFile(astroFile);
        astroFiles[index] = a;
        filePathToIdMap.insert(astroFile.FullPath, a);
        delete existing;
        emit AstroFileUpdated(astroFile, index);
    }
}

void Catalog::pushProcessedQueue()
{
    int local = 0;
    astroFilesQueueMutex.lock();
    if (astroFilesQueue > 0)
    {
        local = astroFilesQueue;
        astroFilesQueue = 0;
    }
    astroFilesQueueMutex.unlock();
    if (local > 0)
    {
        qDebug()<<"Pushing " << local;
        emit AstroFilesAdded(local);
    }
}

void Catalog::addAstroFile(const AstroFile& astroFile)
{
    impAddAstroFile(astroFile, true);
}

void Catalog::addAstroFiles(const QList<AstroFile> &files)
{
    for (auto& a : files)
    {
        if (cancelSignaled)
            return;
        impAddAstroFile(a, true);
    }
//    emit AstroFilesAdded(files.count());
    emit DoneAddingAstrofiles();
}

void Catalog::deleteAstroFile(const AstroFile &astroFile)
{
    QMutexLocker locker(&listMutex);

    int index = astroFileIndex(astroFile);

    if (index == -1)
        return;

    auto a = astroFiles.at(index);
    astroFiles.removeAt(index);
    filePathToIdMap.remove(a->FullPath);
//    emit AstroFileRemoved(astroFile, index);
    delete a;
}

void Catalog::deleteAstroFiles(const QList<AstroFile> &files)
{
    for (auto& a : files)
    {
        deleteAstroFile(a);
    }
}

void Catalog::deleteAstroFileRow(int row)
{
    QMutexLocker locker(&listMutex);
    auto a = astroFiles.at(row);
    astroFiles.removeAt(row);
    filePathToIdMap.remove(a->FullPath);
    delete a;
}

int Catalog::astroFileIndex(const AstroFile &astroFile)
{
    QMutexLocker locker(&listMutex);

    int index = 0;
    for (auto& a : astroFiles)
    {
        if (a->Id == astroFile.Id)
        {
            return index;
        }
        index++;
    }
    return -1;
}

AstroFile* Catalog::getAstroFile(int row)
{
    QMutexLocker locker(&listMutex);
    return astroFiles.at(row);
}

AstroFile *Catalog::getAstroFileByPath(QString path)
{
    QMutexLocker locker(&listMutex);

    if (!filePathToIdMap.contains(path))
        return nullptr;
    return filePathToIdMap[path];
}

bool Catalog::shouldProcessFile(const QFileInfo &fileInfo)
{
    QMutexLocker locker(&listMutex);

    QString path = fileInfo.absoluteFilePath();

    searchFoldersMutex.lock();
    bool isInSearchFolders = false;
    for (auto& s : searchFolders)
    {
        if (path.contains(s))
        {
            isInSearchFolders = true;
            break;
        }
    }
    searchFoldersMutex.unlock();

    if (!isInSearchFolders)
        return false;

    auto a = getAstroFileByPath(path);
    if (a == nullptr)
        return true;

    return (fileInfo.lastModified() > a->LastModifiedTime);
}

int Catalog::getNumberOfItems()
{
    QMutexLocker locker(&listMutex);

    return astroFiles.count();
}
