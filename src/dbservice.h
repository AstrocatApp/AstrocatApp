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

#ifndef DBSERVICE_H
#define DBSERVICE_H

#include "astrofile.h"
#include "filerepository.h"

#include <QList>
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QWaitCondition>

struct OpsNode
{
    QString operation;
    AstroFile astroFile;
    QString path;
    OpsNode(QString o, const AstroFile a, const QString p) : operation(o), astroFile(a), path(p)
    {
    }
};

class DbService : public QObject
{
    Q_OBJECT
public:
    explicit DbService(QObject *parent = nullptr);
    ~DbService();
    void cancel();

public slots:
    void deleteAstrofile(const AstroFile& afi);
    void deleteAstrofilesInFolder(const QString& fullPath);
    void initialize();
    void loadModel();
    void addOrUpdateAstrofile(const AstroFile& afi);
    void getDuplicateFiles();
    void getDuplicateFilesByFileHash();
    void getDuplicateFilesByImageHash();
    void loadThumbnail(const AstroFile& afi);


signals:
    void getAllAstroFilesFinished(const QList<AstroFile>& astroFiles );
    void getTagsFinished(const QMap<QString, QSet<QString>>& tags);
    void astroFileDeleted(const AstroFile& astroFile);
    void astroFilesDeleted(const QList<AstroFile>& astroFiles);
    void modelLoaded(const QList<AstroFile>& astroFile);
    void dbFailedToInitialize(const QString& message);
    void astroFileUpdated(const AstroFile& astroFile);
    void thumbnailLoaded(const AstroFile& astrofile);
    void modelLoadingGotAstrofiles();
    void modelLoadingGotTags();
    void modelLoadingGotThumbnails();

    void FileRepository_deleteAstrofile(const AstroFile& afi);
    void FileRepository_deleteAstrofilesInFolder(const QString& fullPath);
    void FileRepository_initialize();
    void FileRepository_loadModel();
    void FileRepository_addOrUpdateAstrofile(const AstroFile& afi);
    void FileRepository_getDuplicateFiles();
    void FileRepository_getDuplicateFilesByFileHash();
    void FileRepository_getDuplicateFilesByImageHash();
    void FileRepository_loadThumbnail(const AstroFile& afi);

    void DatabaseQueueLength(int length);

private:
    QThread* fileRepositoryThread;
    FileRepository* fileRepository;
    QMutex waitMutex;
    QWaitCondition waitCondition;
    QMutex opsListMutex;
    QList<OpsNode> opsQueue;
    void processLoop();
    QThreadPool threadPool;
    volatile bool cancelSignaled;
    void processOps(OpsNode ops);
};

#endif // DBSERVICE_H
