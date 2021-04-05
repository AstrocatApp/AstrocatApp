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
#include <QDir>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>

#define DB_SCHEMA_VERSION 1

FileRepository::FileRepository(QObject *parent) : QObject(parent)
{

}

void FileRepository::cancel()
{
    cancelSignaled = true;
}

void FileRepository::initialize()
{
    qDebug() << "Initializing File Repository";
    createDatabase();
    migrateDatabase();

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

    auto location = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir{location};
    dir.mkpath(dir.absolutePath());

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName(location + "/astrocat.db");
    if(!db.open())
        qWarning() << "ERROR: " << db.lastError();
    db.exec("PRAGMA foreign_keys = ON");
}

void FileRepository::migrateDatabase()
{
    // Check DB Schema version
    int dbCurrentSchemaVersion = 0;
    QSqlQuery query("PRAGMA user_version");
    query.exec();
    if (query.first())
    {
        dbCurrentSchemaVersion = query.record().value(0).toInt();
        if (dbCurrentSchemaVersion == DB_SCHEMA_VERSION)
        {
            // No migration needed
            return;
        }
    }

    QSqlDatabase::database().transaction();
    migrateFromVersion(dbCurrentSchemaVersion);
    QSqlDatabase::database().commit();
}

void FileRepository::migrateFromVersion(int oldVersion)
{
    switch (oldVersion)
    {
    case 0:
        // This is a new installation. Just create the tables and return
        createTables();
        break;
    default:
        // Should not get here
        break;
    }

    // Set the new schema version
    db.exec(QString("PRAGMA user_version = %1").arg(DB_SCHEMA_VERSION));
}

void FileRepository::createTables()
{
    QSqlQuery fitsquery(
        "CREATE TABLE fits ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "FileName TEXT, "
            "FullPath TEXT, "
            "DirectoryPath TEXT, "
            "FileType TEXT, "
            "FileExtension TEXT, "
            "CreatedTime DATE, "
            "LastModifiedTime DATE, "
            "TagStatus INTEGER, "
            "ThumbnailStatus INTEGER, "
            "ProcessStatus INTEGER,"
            "FileHash TEXT,"
            "ImageHash TEXT)");

    if(!fitsquery.isActive())
        qWarning() << "ERROR: " << fitsquery.lastError().text();

    QSqlQuery fitsFileNameIndexQuery("CREATE UNIQUE INDEX idx_fits_fullpath ON fits (FullPath);");
    if(!fitsFileNameIndexQuery.isActive())
        qWarning() << "ERROR: " << fitsFileNameIndexQuery.lastError().text();

    QSqlQuery tagsquery(
        "CREATE TABLE tags ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "fits_id INTEGER, "
            "tagKey TEXT, "
            "tagValue TEXT, "
            "FOREIGN KEY(fits_id) REFERENCES fits(id) ON DELETE CASCADE)");

    if(!tagsquery.isActive())
        qWarning() << "ERROR: " << tagsquery.lastError().text();

    QSqlQuery thumbnailsquery(
        "CREATE TABLE thumbnails ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "fits_id INTEGER, "
            "thumbnail BLOB, "
            "FOREIGN KEY(fits_id) REFERENCES fits(id) ON DELETE CASCADE)");

    if(!thumbnailsquery.isActive())
        qWarning() << "ERROR: " << thumbnailsquery.lastError().text();
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
    query.exec();
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

QMap<QString, QSet<QString>> GetAllAstrofileTags()
{
    QMap<QString, QSet<QString>> map;
    QSqlQuery query("SELECT tags.tagKey, tags.tagValue FROM tags");
    query.exec();
    while (query.next())
    {
        auto a1 = query.value(0).toString();
        auto a2 = query.value(1).toString();

        if (map.contains(a1))
            map[a1].insert(a2);
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
        int idFileExtension = query.record().indexOf("FileExtension");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        int idFileHash = query.record().indexOf("FileHash");
        int idImageHash = query.record().indexOf("ImageHash");
        AstroFile astro;
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.FileType = AstroFileType(query.value(idFileType).toInt());
        astro.FileExtension = query.value(idFileExtension).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
        astro.FileHash = query.value(idFileHash).toString();
        astro.ImageHash = query.value(idImageHash).toString();

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

void FileRepository::insertAstrofile(const AstroFile& astroFile)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining DB Queue.";
        return;
    }

    QSqlQuery queryAdd;

    queryAdd.prepare("REPLACE INTO fits (FileName,FullPath,DirectoryPath,FileType,FileExtension,CreatedTime,LastModifiedTime, TagStatus, ThumbnailStatus, ProcessStatus, FileHash, ImageHash) "
                        "VALUES (:FileName,:FullPath,:DirectoryPath,:FileType,:FileExtension,:CreatedTime,:LastModifiedTime, :TagStatus, :ThumbnailStatus, :ProcessStatus, :FileHash, :ImageHash)");
    queryAdd.bindValue(":FileName", astroFile.FileName);
    queryAdd.bindValue(":FullPath", astroFile.FullPath);
    queryAdd.bindValue(":DirectoryPath", astroFile.DirectoryPath);
    queryAdd.bindValue(":FileType", astroFile.FileType);
    queryAdd.bindValue(":FileExtension", astroFile.FileExtension);
    queryAdd.bindValue(":CreatedTime", astroFile.CreatedTime);
    queryAdd.bindValue(":LastModifiedTime", astroFile.LastModifiedTime);
    queryAdd.bindValue(":FileHash", astroFile.FileHash);
    queryAdd.bindValue(":ImageHash", astroFile.ImageHash);
    queryAdd.bindValue(":TagStatus", astroFile.tagStatus);
    queryAdd.bindValue(":ThumbnailStatus", astroFile.thumbnailStatus);
    queryAdd.bindValue(":ProcessStatus", astroFile.processStatus);

    if(queryAdd.exec())
    {
        qDebug() << "record added " << astroFile.FullPath;
    }
    else
        qDebug() << "record could not add: " << queryAdd.lastError();
}

void FileRepository::deleteAstrofilesInFolder(const QString fullPath)
{
//    if (cancelSignaled)
//    {
//        qDebug() << "Cancel signaled. Draining DB Queue.";
//        return;
//    }

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
}

void FileRepository::addTags(const AstroFile& astroFile)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining DB Queue.";
        return;
    }

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

    QSqlQuery tagStatusQuery;
    tagStatusQuery.prepare("UPDATE fits set TagStatus = :tagStatus WHERE FullPath = :fullPath");
    tagStatusQuery.bindValue(":tagStatus", astroFile.tagStatus);
    tagStatusQuery.bindValue(":fullPath", astroFile.FullPath);
    if (!tagStatusQuery.exec())
        qDebug() << "FAILED to execute UPDATE TAG Status query: " <<tagStatusQuery.lastError() ;
}

void FileRepository::addThumbnail(const AstroFile &astroFile, const QImage& thumbnail)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining DB Queue.";
        return;
    }

    int id = GetAstroFileId(astroFile.FullPath);

    QByteArray inByteArray;
    QBuffer inBuffer( &inByteArray );
    inBuffer.open( QIODevice::WriteOnly );
    thumbnail.save( &inBuffer, "PNG" );

    QSqlQuery insertThumbnailQuery;
    insertThumbnailQuery.prepare("INSERT INTO thumbnails (fits_id, thumbnail) VALUES (:fits_id, :bytedata)");
    insertThumbnailQuery.bindValue(":fits_id", id);
    insertThumbnailQuery.bindValue(":bytedata", inByteArray);
    if (!insertThumbnailQuery.exec())
        qDebug() << "DB: Failed in insert Thubmanailfor " << astroFile.FullPath << insertThumbnailQuery.lastError();

    QSqlQuery tagStatusQuery;
    tagStatusQuery.prepare("UPDATE fits set ThumbnailStatus = :thumbnailStatus WHERE FullPath = :fullPath");
    tagStatusQuery.bindValue(":thumbnailStatus", astroFile.thumbnailStatus);
    tagStatusQuery.bindValue(":fullPath", astroFile.FullPath);
    if (!tagStatusQuery.exec())
        qDebug() << "FAILED to execute UPDATE Thumbnail Status query" << tagStatusQuery.lastError();
}

void FileRepository::saveStatus(const AstroFile& astroFile)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining DB Queue.";
        return;
    }

    QSqlQuery processStatusQuery;
    processStatusQuery.prepare("UPDATE fits set ProcessStatus = :processStatus WHERE FullPath = :fullPath");
    processStatusQuery.bindValue(":processStatus", astroFile.processStatus);
    processStatusQuery.bindValue(":fullPath", astroFile.FullPath);
    if (!processStatusQuery.exec())
        qDebug() << "FAILED to execute UPDATE TAG Status query: " <<processStatusQuery.lastError() ;
}

void FileRepository::getDuplicateFiles()
{
    qDebug()<<"By File Hash:";
    getDuplicateFilesByFileHash();
    qDebug()<<"By Image Hash:";
    getDuplicateFilesByImageHash();
}
void FileRepository::getDuplicateFilesByFileHash()
{
    QSqlQuery query("SELECT FullPath, COUNT(*) c FROM fits GROUP BY FileHash HAVING c > 1");
    query.exec();
    int idCount = query.record().indexOf("c");
    int idFullPath = query.record().indexOf("FullPath");

    while (query.next())
    {
        int count = query.value(idCount).toInt();
        QString fullPath = query.value(idFullPath).toString();

        qDebug()<<"Duplicate: " << fullPath << " Count: " << count;
    }
}
void FileRepository::getDuplicateFilesByImageHash()
{
    QSqlQuery query("SELECT FullPath, COUNT(*) c FROM fits GROUP BY ImageHash HAVING c > 1");
    query.exec();
    int idCount = query.record().indexOf("c");
    int idFullPath = query.record().indexOf("FullPath");

    while (query.next())
    {
        int count = query.value(idCount).toInt();
        QString fullPath = query.value(idFullPath).toString();

        qDebug()<<"Duplicate: " << fullPath << " Count: " << count;
    }
}
QMap<int, AstroFile> FileRepository::_getAllAstrofiles()
{
    QSqlQuery query("SELECT * FROM fits");
    query.exec();
    int idId = query.record().indexOf("Id");
    int idFileName = query.record().indexOf("FileName");
    int idFullPath = query.record().indexOf("FullPath");
    int idDirectoryPath = query.record().indexOf("DirectoryPath");
    int idFileType = query.record().indexOf("FileType");
    int idFileExtension = query.record().indexOf("FileExtension");
    int idCreatedTime = query.record().indexOf("CreatedTime");
    int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
    int idFileHash = query.record().indexOf("FileHash");
    int idImageHash = query.record().indexOf("ImageHash");
    int idTagStatus = query.record().indexOf("TagStatus");
    int idThumbnailStatus = query.record().indexOf("ThumbnailStatus");
    int idProcessStatus = query.record().indexOf("ProcessStatus");

    QMap<int, AstroFile> files;
    while (query.next())
    {
        AstroFile astro;
        int astroFileId = query.value(idId).toInt();
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.FileType = AstroFileType(query.value(idFileType).toInt());
        astro.FileExtension = query.value(idFileExtension).toString();
        astro.FileHash = query.value(idFileHash).toString();
        astro.ImageHash = query.value(idImageHash).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
        astro.thumbnailStatus = ThumbnailLoadStatus(query.value(idThumbnailStatus).toInt());
        astro.tagStatus = TagExtractStatus(query.value(idTagStatus).toInt());
        astro.processStatus = AstroFileProcessStatus(query.value(idProcessStatus).toInt());

        files.insert(astroFileId, astro);
    }

    return files;
}

QMap<int, QMap<QString, QString>> _getAllAstrofileTags()
{
    QSqlQuery query("SELECT * FROM tags");
    query.exec();
    int fits_idId = query.record().indexOf("fits_id");
    int idtagKey = query.record().indexOf("tagKey");
    int idtagValue = query.record().indexOf("tagValue");

    QMap<int, QMap<QString, QString>> map;
    while (query.next())
    {
        auto fitsId = query.value(fits_idId).toInt();
        auto tagKey = query.value(idtagKey).toString();
        auto tagValue = query.value(idtagValue).toString();

        map[fitsId][tagKey] = tagValue;
    }
    return map;
}

QMap<int, QImage> FileRepository::_getAllThumbnails()
{
    QSqlQuery query("SELECT * FROM thumbnails");
    query.exec();
    int fits_idId = query.record().indexOf("fits_id");
    int idThumbnail = query.record().indexOf("thumbnail");

    QMap<int, QImage> images;
    while (query.next())
    {
        AstroFile astro;
        int fitsId = query.value(fits_idId).toInt();
        QByteArray inByteArray = query.value(idThumbnail).toByteArray();

        images[fitsId].loadFromData(inByteArray, "PNG");
    }

    return images;
}

void FileRepository::loadModel()
{
    // 1. Get the entire fits table into memory
    // select * from fits
    // create a map with key (fits_id), and value Astrofile
    // insert all into map

    auto fitsmap = _getAllAstrofiles();

    // 2. Get the entire tags table into memory
    // select * from tags

    auto tagsmap = _getAllAstrofileTags();

    // 3. Add tags to their fits files
    // insert all tags from #2 into map by fits_id

    QMapIterator<int, QMap<QString, QString>> iter(tagsmap);

    while(iter.hasNext())
    {
        iter.next();
        auto fitsId = iter.key();
        auto& tagsList = iter.value();
        fitsmap[fitsId].Tags.insert(tagsList);
    }

    // 4. Get the entire thumbnails into memory
    // select * from thumbnails

    auto thumbnails = _getAllThumbnails();

    // 5. Add thumbnails to their fits files
    // insert all thumbnails from #4 into map by fits_id

    QMapIterator<int, QImage> thumbiter(thumbnails);
    while (thumbiter.hasNext())
    {
        thumbiter.next();
        auto fitsId = thumbiter.key();
        auto& image = thumbiter.value();
        fitsmap[fitsId].thumbnail = image;
        fitsmap[fitsId].thumbnailStatus = Loaded;
    }

    // 6. convert map's `values` into a list of astroFile, and emit the list

    auto list = fitsmap.values();

    emit modelLoaded(list);
}
