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

#ifndef FILEREPOSITORY_H
#define FILEREPOSITORY_H

#include "astrofile.h"

#include <QObject>
#include <QSqlDatabase>

class FileRepository : public QObject
{
    Q_OBJECT
public:
    FileRepository(QObject *parent = nullptr);

public slots:
    void getAstrofile(const QString fullPath);
    void getAllAstrofiles();
    void insertAstrofile(const AstroFile& astroFile);
    void deleteAstrofilesInFolder(const QString fullPath);
    void deleteAstrofile(const AstroFile& astroFile);
    void initialize();
    void addTags(const AstroFile& astroFile);
    void addThumbnail(const AstroFile& astroFile, const QImage& thumbnail);
    void getThumbnails();
    void getThumbnail(const QString fullPath);
    void getTags();

signals:
    void getAstroFileFinished(const AstroFile& astroFile );
    void getAllAstroFilesFinished(const QList<AstroFile>& astroFiles );
    void getThumbnailFinished(const AstroFile& astroFile, const QPixmap& thumbnail );
    void getTagsFinished(const QMap<QString, QSet<QString>>& tags);
    void astroFileDeleted(const AstroFile& astroFile);

private:
    QSqlDatabase db;
    void createTables();
    void createDatabase();
    void initializeTables();
    QList<AstroFile> getAstrofilesInFolder(const QString fullPath, bool includeTags);
};

#endif // FILEREPOSITORY_H
