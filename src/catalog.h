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

#ifndef CATALOG_H
#define CATALOG_H

#include "astrofile.h"

#include <QObject>
#include <QFileInfo>
#include <QRecursiveMutex>
#include <QTimer>

class Catalog : public QObject
{
    Q_OBJECT
public:
    explicit Catalog(QObject *parent = nullptr);
    ~Catalog();

    void addSearchFolder(const QString& folder);
    void addSearchFolder(const QList<QString>& folders);
    void removeSearchFolder(const QString& folder);
    void removeAllSearchFolders();
    void cancel();


//    void hideAstroFile(const AstroFile& astroFile);
//    void unhideAstroFile(const AstroFile& astroFile);

//    void removeAstroFilesInFolder(const QString& folder);

//    /*
//     * The FileProcessFilter will call this.
//     * There might be a large number of incoming requests for this from the FileProcessFilter
//     * while there is also a large number of requests for "addAstroFile"
//     * coming from the db.
//     */
    AstroFileCatalogStatus shouldProcessFile(const QFileInfo& fileInfo);


    int getNumberOfItems();
    int astroFileIndex(const AstroFile& astroFile); // Returns the 0-based row number of the object. -1 on failure
    AstroFile* getAstroFile(int row);

public slots:
    void addAstroFile(const AstroFile& astroFile);
    void addAstroFiles(const QList<AstroFile>& files);

//    void deleteAstroFile(const AstroFile& astroFile);
//    void deleteAstroFiles(const QList<AstroFile>& files);
    void deleteAstroFileRow(int row);

signals:
//    void AstroFileAdded(AstroFile astroFile, int row);
    void AstroFilesAdded(int numberAdded);
    void AstroFileUpdated(AstroFile astroFile, int row);
    void AstroFileRemoved(AstroFile astroFile, int row);
    void DoneAddingAstrofiles();

private:
    QRecursiveMutex listMutex;
    QRecursiveMutex searchFoldersMutex;

    QList<QString> searchFolders;
    QList<AstroFile*> astroFiles;
    QMap<QString, AstroFile*> filePathToIdMap;

    AstroFile* getAstroFileByPath(QString path);
    void impAddAstroFile(const AstroFile& astroFile, bool shouldEmit = true);
    QTimer timer;
    void pushProcessedQueue();
    QRecursiveMutex astroFilesQueueMutex;
    int astroFilesQueue;
    volatile bool cancelSignaled = false;
};

#endif // CATALOG_H
