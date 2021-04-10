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


// We should check if the folder is a child folder of an
// existing search folder.
// Or if we add a parent folder, we might want to remove
// child folders
Catalog::Catalog(QObject *parent)
{

}

Catalog::~Catalog()
{

}

void Catalog::addSearchFolder(const QString &folder)
{
    searchFolders.append(folder);
}

void Catalog::addSearchFolder(const QList<QString> &folders)
{
    searchFolders.append(folders);
}

void Catalog::impAddAstroFile(const AstroFile &astroFile, bool shouldEmit)
{
    QMutexLocker locker(&listMutex);

    // Check if this file already exists
    // We should probably check by id, but we don't have that mapping

    auto existing = getAstroFileByPath(astroFile.FullPath);
    if (existing == nullptr)
    {
        AstroFile* a = new AstroFile(astroFile);
        astroFiles.append(a);
        filePathToIdMap.insert(astroFile.FullPath, a);
        if (shouldEmit)
            emit AstroFilesAdded(1);
    }
    else
    {
        int index = astroFileIndex(astroFile);
        AstroFile* a = new AstroFile(astroFile);
        astroFiles[index] = a;
        filePathToIdMap.insert(astroFile.FullPath, a);
        delete existing;
        emit AstroFileUpdated(astroFile, index);
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
        impAddAstroFile(a, false);
    }
    emit AstroFilesAdded(files.count());
}

void Catalog::deleteAstroFile(const AstroFile &astroFile)
{
    QMutexLocker locker(&listMutex);

    int index = astroFileIndex(astroFile);

    if (index == -1)
        return;

    auto a = astroFiles.at(index);
    astroFiles.removeAt(index);
    filePathToIdMap.remove(astroFile.FullPath);
    emit AstroFileRemoved(astroFile, index);
    delete a;
}

void Catalog::deleteAstroFiles(const QList<AstroFile> &files)
{
    for (auto& a : files)
    {
        deleteAstroFile(a);
    }
}

int Catalog::astroFileIndex(const AstroFile &astroFile)
{
    QMutexLocker locker(&listMutex);

    int index = 0;
    for (auto& a : astroFiles)
    {
        if (a->Id == astroFile.Id)
            return index;
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
