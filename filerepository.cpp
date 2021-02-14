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

#include "filerepository.h"

#include <QBuffer>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

FileRepository::FileRepository(QObject *parent) : QObject(parent)
{

}

void FileRepository::initialize()
{
    qDebug() << "Initializing File Repository";
    createDatabase();
    createTables();
    initializeTables();

     qDebug() << "Done Initializing File Repository";
}

void FileRepository::createDatabase()
{
    const QString DRIVER("QSQLITE");
    if(!QSqlDatabase::isDriverAvailable(DRIVER))
    {
        qDebug()<<"SQLite Driver not available";
        return;
    }

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName("astrocat.db");
    if(!db.open())
        qWarning() << "ERROR: " << db.lastError();

}

void FileRepository::createTables()
{
    QSqlQuery fitsquery("CREATE TABLE fits (id INTEGER PRIMARY KEY AUTOINCREMENT, FileName TEXT, FullPath TEXT, DirectoryPath TEXT, FileType TEXT, CreatedTime DATE, LastModifiedTime DATE)");
    fitsquery.exec("PRAGMA foreign_keys = ON");
    if(!fitsquery.isActive())
        qWarning() << "ERROR: " << fitsquery.lastError().text();

    QSqlQuery tagsquery("CREATE TABLE tags (id INTEGER PRIMARY KEY AUTOINCREMENT, fits_id INTEGER, tagKey TEXT, tagValue TEXT, FOREIGN KEY(fits_id) REFERENCES fits(id) ON DELETE CASCADE)");
    tagsquery.exec("PRAGMA foreign_keys = ON");
    if(!tagsquery.isActive())
        qWarning() << "ERROR: " << tagsquery.lastError().text();

    QSqlQuery thumbnailsquery("CREATE TABLE thumbnails (id INTEGER PRIMARY KEY AUTOINCREMENT, fits_id INTEGER, thumbnail BLOB, FOREIGN KEY(fits_id) REFERENCES fits(id) ON DELETE CASCADE)");
    thumbnailsquery.exec("PRAGMA foreign_keys = ON");
    if(!thumbnailsquery.isActive())
        qWarning() << "ERROR: " << thumbnailsquery.lastError().text();
}

void FileRepository::initializeTables()
{
}

int GetAstroFileId(const QString fullPath)
{
    int id = 0;
    QSqlQuery query("SELECT id FROM fits WHERE FullPath = ?");
    query.bindValue(0, fullPath);
    query.exec();
    if (query.first())
    {
        id = query.record().value(0).toInt();
    }
    return id;
}

QMap<QString, QString> GetAstrofileTags(int astroFileId)
{
    QMap<QString, QString> map;
    QSqlQuery query("SELECT * FROM tags WHERE fits_id = ?");
    query.bindValue(0, astroFileId);
    int err = query.exec();
    while (query.next())
    {
        int idtagKey = query.record().indexOf("tagKey");
        int idtagValue = query.record().indexOf("tagValue");
        auto key = query.value(idtagKey).toString();
        auto value = query.value(idtagValue).toString();
        map.insert(key, value);
    }
    return map;
}

//QMap<QString, QString> GetAllAstrofileTags()
QMap<QString, QSet<QString>> GetAllAstrofileTags()
{
    QMap<QString, QSet<QString>> map;
    QSqlQuery query("SELECT tags.tagKey, tags.tagValue FROM tags");
//    QSqlQuery query("SELECT tags.tagKey, COUNT(DISTINCT tags.tagValue) FROM tags GROUP BY tags.tagKey");
    int err = query.exec();
    while (query.next())
    {
        auto a1 = query.value(0).toString();
        auto a2 = query.value(1).toString();
//        map.insert(a1, a2);
        if (map.contains(a1))
        {
            map[a1].insert(a2);
        }
        else
            map.insert(a1, QSet<QString>({a2}));
    }
    return map;
}

QList<AstroFile> FileRepository::getAstrofilesInFolder(const QString fullPath, bool includeTags)
{
    QList<AstroFile> files;
    QSqlQuery query;
    QString paddedFullPath;
    // If the full path does not end with a '/', append it, otherwise the `LIKE` statement
    // may match other folders that start with the same name.
    if (fullPath.endsWith('/'))
        paddedFullPath = fullPath;
    else
        paddedFullPath = fullPath + '/';
    query.prepare("SELECT * FROM fits WHERE FullPath LIKE :fullPathString");
    query.bindValue(":fullPathString", QString("%%1%").arg(paddedFullPath));

    bool ret = query.exec();
    if (!ret)
    {
        qDebug() << "could not query: " << query.lastError();
        return files;
    }

    while (query.next())
    {
        int idId = query.record().indexOf("Id");
        int idFileName = query.record().indexOf("FileName");
        int idFullPath = query.record().indexOf("FullPath");
        int idDirectoryPath = query.record().indexOf("DirectoryPath");
        int idFileType = query.record().indexOf("FileType");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        AstroFile astro;
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.FileType = query.value(idFileType).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();

        int astroFileId = query.value(idId).toInt();
        if (includeTags)
        {
            auto map = GetAstrofileTags(astroFileId);
            astro.Tags.insert(map);
        }
        files.append(astro);
    }

    return files;
}

void FileRepository::getAstrofile(const QString fullPath)
{
    QSqlQuery query("SELECT * FROM fits WHERE FullPath = ?");
    query.bindValue(0, fullPath);
    query.exec();
    if (query.first())
    {
        int idId = query.record().indexOf("Id");
        int idFileName = query.record().indexOf("FileName");
        int idFullPath = query.record().indexOf("FullPath");
        int idDirectoryPath = query.record().indexOf("DirectoryPath");
        int idFileType = query.record().indexOf("FileType");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        AstroFile astro;
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.FileType = query.value(idFileType).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();

        int astroFileId = query.value(idId).toInt();
        auto map = GetAstrofileTags(astroFileId);
        astro.Tags.insert(map);

        emit getAstroFileFinished( astro );

        return;
    }

    return;
}

void FileRepository::getAllAstrofiles()
{
    QSqlQuery query("SELECT * FROM fits");
    query.exec();
    QList<AstroFile> files;
    while (query.next())
    {
        int idId = query.record().indexOf("Id");
        int idFileName = query.record().indexOf("FileName");
        int idFullPath = query.record().indexOf("FullPath");
        int idDirectoryPath = query.record().indexOf("DirectoryPath");
        int idFileType = query.record().indexOf("FileType");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        AstroFile astro;
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.FileType = query.value(idFileType).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();

        int astroFileId = query.value(idId).toInt();
        auto map = GetAstrofileTags(astroFileId);
        astro.Tags.insert(map);

        files.append(astro);
    }

    emit getAllAstroFilesFinished(files);
    return;
}

void FileRepository::insertAstrofile(const AstroFile& astroFile)
{
    bool success = false;
    QSqlQuery queryAdd;
    queryAdd.prepare("INSERT INTO fits (FileName,FullPath,DirectoryPath,FileType,CreatedTime,LastModifiedTime) VALUES (:FileName,:FullPath,:DirectoryPath,:FileType,:CreatedTime,:LastModifiedTime)");
    queryAdd.bindValue(":FileName", astroFile.FileName);
    queryAdd.bindValue(":FullPath", astroFile.FullPath);
    queryAdd.bindValue(":DirectoryPath", astroFile.DirectoryPath);
    queryAdd.bindValue(":FileType", astroFile.FileType);
    queryAdd.bindValue(":CreatedTime", astroFile.CreatedTime);
    queryAdd.bindValue(":LastModifiedTime", astroFile.LastModifiedTime);

    if(queryAdd.exec())
    {
        success = true;
        qDebug() << "record added " << astroFile.FullPath;
    }
    else
        qDebug() << "record could not add: " << queryAdd.lastError();

    return;
}

void FileRepository::deleteAstrofile(const AstroFile& astroFile)
{
    QSqlQuery query;
    query.prepare("DELETE FROM fits WHERE fullPath = :astrofilePath");
    query.bindValue(":astrofileId", astroFile.FullPath);
    bool ret = query.exec();
    if (!ret)
        qDebug() << "could not delete: " << query.lastError();

    return;
}

void FileRepository::deleteAstrofilesInFolder(const QString fullPath)
{
    auto files = getAstrofilesInFolder(fullPath, false);
    QSqlQuery query;
    QString paddedFullPath;
    // If the full path does not end with a '/', append it, otherwise the `LIKE` statement
    // may match other folders that start with the same name.
    if (fullPath.endsWith('/'))
        paddedFullPath = fullPath;
    else
        paddedFullPath = fullPath + '/';

    query.prepare("DELETE FROM fits WHERE FullPath LIKE :fullPathString");
    query.bindValue(":fullPathString", QString("%%1%").arg(paddedFullPath));
    bool ret = query.exec();
    if (!ret)
        qDebug() << "could not delete: " << query.lastError();

    for(auto& file : files)
    {
        emit astroFileDeleted(file);
    }
    return;
}

void FileRepository::addTags(const AstroFile& astroFile)
{
    int id = GetAstroFileId(astroFile.FullPath);
    for (auto iter = astroFile.Tags.constBegin(); iter != astroFile.Tags.constEnd(); ++iter)
    {
        auto& key = iter.key();
        auto& value = iter.value();

        QSqlQuery tagAddQuery;
        tagAddQuery.prepare("INSERT INTO tags (fits_id,tagKey,tagValue) VALUES (:fits_id,:tagKey,:tagValue)");
        tagAddQuery.bindValue(":fits_id", id);
        tagAddQuery.bindValue(":tagKey", key);
        tagAddQuery.bindValue(":tagValue", value);
        if (!tagAddQuery.exec())
            qDebug() << "FAILED to execute INSERT TAG query";
    }
    return;
}

void FileRepository::addThumbnail(const AstroFile &astroFile, const QImage& thumbnail)
{
    int id = GetAstroFileId(astroFile.FullPath);

    QPixmap inPixmap = QPixmap().fromImage(thumbnail);
    QByteArray inByteArray;
    QBuffer inBuffer( &inByteArray );
    inBuffer.open( QIODevice::WriteOnly );
    inPixmap.save( &inBuffer, "PNG" ); // write inPixmap into inByteArray in PNG format

    QSqlQuery insertThumbnailQuery;
    insertThumbnailQuery.prepare("INSERT INTO thumbnails (fits_id, thumbnail) VALUES (:fits_id, :bytedata)");
    insertThumbnailQuery.bindValue(":fits_id", id);
    insertThumbnailQuery.bindValue(":bytedata", inByteArray);
    if (!insertThumbnailQuery.exec())
        qDebug() << "DB: Failed in insert Thubmanailfor " << astroFile.FullPath << insertThumbnailQuery.lastError();
}

void FileRepository::getThumbnails()
{
    QSqlQuery query("SELECT fits.*, thumbnails.thumbnail FROM fits LEFT JOIN thumbnails ON thumbnails.fits_id=fits.id");
    if (!query.exec())
    {
        qDebug() << "query failed: " << query.lastError();
        return;
    }
    while (query.next())
    {
        AstroFile astro;

        auto x = query.record();
        int idId = query.record().indexOf("Id");
        int idFileName = query.record().indexOf("FileName");
        int idFullPath = query.record().indexOf("FullPath");
        int idDirectoryPath = query.record().indexOf("DirectoryPath");
        int idFileType = query.record().indexOf("FileType");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        int idThumbnail = query.record().indexOf("thumbnail");
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.FileType = query.value(idFileType).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
        QByteArray inByteArray = query.value(idThumbnail).toByteArray();

        QPixmap outPixmap;
        outPixmap.loadFromData(inByteArray, "PNG");

        emit getThumbnailFinished(astro, outPixmap);
    }

    return;
}

void FileRepository::getThumbnail(const QString fullPath)
{
    QSqlQuery query("SELECT fits.*, thumbnails.thumbnail FROM fits LEFT JOIN thumbnails ON thumbnails.fits_id=fits.id WHERE fits.FullPath = ?");
    query.bindValue(0, fullPath);
    if (!query.exec())
    {
        qDebug() << "query failed: " << query.lastError();
        return;
    }
    if (query.next())
    {
        AstroFile* astro = new AstroFile();
        auto x = query.record();
        int idId = query.record().indexOf("Id");
        int idFileName = query.record().indexOf("FileName");
        int idFullPath = query.record().indexOf("FullPath");
        int idDirectoryPath = query.record().indexOf("DirectoryPath");
        int idFileType = query.record().indexOf("FileType");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        int idThumbnail = query.record().indexOf("thumbnail");

        astro->FileName = query.value(idFileName).toString();
        astro->FullPath = query.value(idFullPath).toString();
        astro->DirectoryPath = query.value(idDirectoryPath).toString();
        astro->FileType = query.value(idFileType).toString();
        astro->CreatedTime = query.value(idCreatedTime).toDateTime();
        astro->LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
        QByteArray inByteArray = query.value(idThumbnail).toByteArray();

        int astroFileId = query.value(idId).toInt();
        auto map = GetAstrofileTags(astroFileId);
        astro->Tags.insert(map);

        QPixmap outPixmap;
        outPixmap.loadFromData(inByteArray, "PNG");

        emit getThumbnailFinished(*astro, outPixmap );

        return;
    }

    return;
}

void FileRepository::getTags()
{
    auto map = GetAllAstrofileTags();
    emit getTagsFinished(map);
}
