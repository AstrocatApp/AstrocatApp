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

FileRepository::FileRepository(QString connectionName, QObject *parent) : QObject(parent)
{
    this->connectionName = connectionName;
}

void FileRepository::cancel()
{
    cancelSignaled = true;
}

/*!
 * \brief FileRepository::initialize
 * Creates the Catalog Database if it does not exist, otherwise opens it.
 * If the database schema is from a previous version that needs to be updated,
 * then it performs the database migration.
 *
 * If the database cannot be creaetd, opened, or migrated, the dbFailedToOpen
 * signal will be emitted.
 *
 * Databse file location:
 *  MacOS:      /Users/<username>/Library/Application Support/Astrocat/Astrocat/astrocat.db
 *  Windows:
 *  Linux:      /home/<username>/.local/share/Astrocat/Astrocat/astrocat.db
 */
void FileRepository::initialize()
{
    qDebug() << "Initializing File Repository";
    createDatabase();
    migrateDatabase();
    qDebug() << "Done Initializing File Repository";
}

/*!
 * \brief FileRepository::createDatabase
 * This is a private function.
 * Creates the Database.
 * Sets Foreign keys policy at the database level, so we don't have to
 * set it for individual tables.
 * Tables are not created here. They will be handled by the migrateDatabase function
 */
void FileRepository::createDatabase()
{
    const QString DRIVER("QSQLITE");
    if(!QSqlDatabase::isDriverAvailable(DRIVER))
    {
        emit dbFailedToInitialize("SQLite Driver not available");
        return;
    }

    auto location = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir{location};
    dir.mkpath(dir.absolutePath());

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName(location + "/astrocat.db");
//    db.setDatabaseName(":memory:");
    if(!db.open())
    {
        auto message = QString("db.open() failed: %1").arg(db.lastError().text());
        emit dbFailedToInitialize(message);
        return;
    }
    db.exec("PRAGMA foreign_keys = ON");
    db.exec("PRAGMA cache_size = -100000");
}

/*!
 * \brief FileRepository::migrateDatabase
 * This is a private function.
 * If a database from a previous version already exists on this machine,
 * and if the schema has changed, then migrates the old database to the
 * new schema.
 */
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

/*!
 * \brief FileRepository::migrateFromVersion
 * \param oldVersion
 *
 * Migrates the Catalog database which was created by schema version oldVersion
 * If there was not databse found at all (if this is the first time we are running
 * on this machine), then oldVersion should be set to 0 for new tables to be created.
 */
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

/*!
 * \brief FileRepository::createTables
 * Creates all tables for the Database.
 * We are using three tables: FITS, TAGS, and THUMBNAILS
 *
 * Application settings are not stored in the Database.
 * If this function fails, we will emit the dbFailedToInitialize signal
 */
void FileRepository::createTables()
{
    QSqlQuery fitsquery(
        "CREATE TABLE fits ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "FileName TEXT, "
            "FullPath TEXT, "
            "DirectoryPath TEXT, "
            "VolumeName TEXT, "
            "VolumeRoot TEXT, "
            "FileType TEXT, "
            "FileExtension TEXT, "
            "CreatedTime DATE, "
            "LastModifiedTime DATE, "
            "TagStatus INTEGER, "
            "ThumbnailStatus INTEGER, "
            "ProcessStatus INTEGER,"
            "FileHash TEXT,"
            "ImageHash TEXT,"
            "IsHidden INTEGER)");

    if(!fitsquery.isActive())
    {
        emit dbFailedToInitialize(fitsquery.lastError().text());
        return;
    }

    QSqlQuery fitsFileNameIndexQuery("CREATE UNIQUE INDEX idx_fits_fullpath ON fits(FullPath);");
    if(!fitsFileNameIndexQuery.isActive())
    {
        emit dbFailedToInitialize(fitsFileNameIndexQuery.lastError().text());
        return;
    }

    QSqlQuery tagsquery(
        "CREATE TABLE tags ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "fits_id INTEGER, "
            "tagKey TEXT, "
            "tagValue TEXT, "
            "FOREIGN KEY(fits_id) REFERENCES fits(id) ON DELETE CASCADE)");

    if(!tagsquery.isActive())
    {
        emit dbFailedToInitialize(tagsquery.lastError().text());
        return;
    }

    QSqlQuery tagsFitsIdIndexQuery("CREATE INDEX idx_ftags_fitsid ON tags(fits_id);");
    if(!tagsFitsIdIndexQuery.isActive())
    {
        emit dbFailedToInitialize(tagsFitsIdIndexQuery.lastError().text());
        return;
    }

    QSqlQuery thumbnailsquery(
        "CREATE TABLE thumbnails ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "fits_id INTEGER, "
            "thumbnail BLOB, "
            "tiny_thumbnail BLOB, "
            "FOREIGN KEY(fits_id) REFERENCES fits(id) ON DELETE CASCADE)");

    if(!thumbnailsquery.isActive())
    {
        emit dbFailedToInitialize(thumbnailsquery.lastError().text());
        return;
    }

    QSqlQuery thumbnailsFitsIdIndexQuery("CREATE UNIQUE INDEX idx_thumbnails_fitsid ON thumbnails(fits_id);");
    if(!thumbnailsFitsIdIndexQuery.isActive())
    {
        emit dbFailedToInitialize(thumbnailsFitsIdIndexQuery.lastError().text());
        return;
    }
}

QList<AstroFile> FileRepository::getAstrofilesInFolder(const QString& fullPath)
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
        int idVolumeName = query.record().indexOf("VolumeName");
        int idVolumeRoot = query.record().indexOf("VolumeRoot");
        int idFileType = query.record().indexOf("FileType");
        int idFileExtension = query.record().indexOf("FileExtension");
        int idCreatedTime = query.record().indexOf("CreatedTime");
        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
        int idFileHash = query.record().indexOf("FileHash");
        int idImageHash = query.record().indexOf("ImageHash");
        int idIsHidden = query.record().indexOf("IsHidden");
        AstroFile astro;
        astro.Id = query.value(idId).toInt();
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.VolumeName = query.value(idVolumeName).toString();
        astro.VolumeRoot = query.value(idVolumeRoot).toString();
        astro.FileType = AstroFileType(query.value(idFileType).toInt());
        astro.FileExtension = query.value(idFileExtension).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
        astro.FileHash = query.value(idFileHash).toString();
        astro.ImageHash = query.value(idImageHash).toString();
        astro.IsHidden = query.value(idIsHidden).toInt();

        files.append(astro);
    }

    return files;
}

void FileRepository::addAstrofile(const AstroFile& astroFile)
{
    QSqlDatabase::database().transaction();

    int id = insertAstrofile(astroFile);
    AstroFile insertedAstroFile(astroFile);
    insertedAstroFile.Id = id;

    addTags(insertedAstroFile);
    addThumbnail(insertedAstroFile);

    QSqlDatabase::database().commit();

    // Since we wrote the full thumbnail to the DB,
    // we do not need to keep it in memory since the image
    // may not be visible on screen. If it is needed,
    // the view will ask for it.
    insertedAstroFile.thumbnail = QImage();
    emit astroFileUpdated(insertedAstroFile);
}

int FileRepository::insertAstrofile(const AstroFile& astroFile)
{
    QSqlQuery queryAdd;

    // We changed the following from REPLACE to INSERT
    queryAdd.prepare("INSERT INTO fits (FileName,FullPath,DirectoryPath,VolumeName,VolumeRoot,FileType,FileExtension,CreatedTime,LastModifiedTime,TagStatus,ThumbnailStatus,ProcessStatus,FileHash,ImageHash,IsHidden) "
                        "VALUES (:FileName,:FullPath,:DirectoryPath,:VolumeName,:VolumeRoot,:FileType,:FileExtension,:CreatedTime,:LastModifiedTime,:TagStatus,:ThumbnailStatus,:ProcessStatus,:FileHash,:ImageHash,:IsHidden)");
    queryAdd.bindValue(":FileName", astroFile.FileName);
    queryAdd.bindValue(":FullPath", astroFile.FullPath);
    queryAdd.bindValue(":DirectoryPath", astroFile.DirectoryPath);
    queryAdd.bindValue(":VolumeName", astroFile.VolumeName);
    queryAdd.bindValue(":VolumeRoot", astroFile.VolumeRoot);
    queryAdd.bindValue(":FileType", astroFile.FileType);
    queryAdd.bindValue(":FileExtension", astroFile.FileExtension);
    queryAdd.bindValue(":CreatedTime", astroFile.CreatedTime);
    queryAdd.bindValue(":LastModifiedTime", astroFile.LastModifiedTime);
    queryAdd.bindValue(":FileHash", astroFile.FileHash);
    queryAdd.bindValue(":ImageHash", astroFile.ImageHash);
    queryAdd.bindValue(":TagStatus", astroFile.tagStatus);
    queryAdd.bindValue(":ThumbnailStatus", astroFile.thumbnailStatus);
    queryAdd.bindValue(":ProcessStatus", astroFile.processStatus);
    queryAdd.bindValue(":IsHidden", astroFile.IsHidden);

    if(!queryAdd.exec())
    {
        // TODO: Handle failures
        qDebug() << "record could not add: " << queryAdd.lastError();
        return 0;
    }

    return queryAdd.lastInsertId().toInt();
}

void FileRepository::deleteAstrofile(const AstroFile& afi)
{
    QSqlQuery query;

    query.prepare("DELETE FROM fits WHERE FullPath LIKE :fullPathString");
    auto queryPath = afi.FullPath;
    query.bindValue(":fullPathString", queryPath);
    bool ret = query.exec();
    if (!ret)
        qDebug() << "could not delete: " << query.lastError();

    emit astroFileDeleted(afi);
}

void FileRepository::deleteAstrofilesInFolder(const QString& fullPath)
{
    auto files = getAstrofilesInFolder(fullPath);
    QSqlQuery query;
    QString paddedFullPath;

    // If the full path does not end with a '/', append it, otherwise the `LIKE` statement
    // may match other folders that start with the same name.
    if (fullPath.endsWith('/'))
        paddedFullPath = fullPath;
    else
        paddedFullPath = fullPath + '/';

    query.prepare("DELETE FROM fits WHERE FullPath LIKE :fullPathString");
    auto queryPath = QString("%1%").arg(paddedFullPath);
//    qDebug()<<queryPath;
    query.bindValue(":fullPathString", queryPath);
    bool ret = query.exec();
    if (!ret)
        qDebug() << "could not delete: " << query.lastError();

    qDebug()<<"Done deleting from table";
    emit astroFilesDeleted(files);
    qDebug()<<"Done deleting";
}

void FileRepository::addTags(const AstroFile& astroFile)
{
    int id = astroFile.Id;
    Q_ASSERT(id != 0);

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
            qDebug() << "FAILED to execute INSERT TAG query: " << tagAddQuery.lastError();
    }

    QSqlQuery tagStatusQuery;
    tagStatusQuery.prepare("UPDATE fits set TagStatus = :tagStatus WHERE FullPath = :fullPath");
    tagStatusQuery.bindValue(":tagStatus", astroFile.tagStatus);
    tagStatusQuery.bindValue(":fullPath", astroFile.FullPath);
    if (!tagStatusQuery.exec())
        qDebug() << "FAILED to execute UPDATE TAG Status query: " <<tagStatusQuery.lastError() ;
}

void FileRepository::addThumbnail(const AstroFile &astroFile)
{
    int id = astroFile.Id;
    Q_ASSERT(id != 0);

    QByteArray inByteArray;
    QBuffer inBuffer( &inByteArray );
    inBuffer.open( QIODevice::WriteOnly );
    astroFile.thumbnail.save( &inBuffer, "PNG" );

    QByteArray inByteArrayTiny;
    QBuffer inBufferTiny( &inByteArrayTiny );
    inBufferTiny.open( QIODevice::WriteOnly );
    astroFile.tinyThumbnail.save( &inBufferTiny, "PNG" );


    QSqlQuery insertThumbnailQuery;
    insertThumbnailQuery.prepare("INSERT INTO thumbnails (fits_id, thumbnail, tiny_thumbnail) VALUES (:fits_id, :bytedata, :tinyThumbnail)");
    insertThumbnailQuery.bindValue(":fits_id", id);
    insertThumbnailQuery.bindValue(":bytedata", inByteArray);
    insertThumbnailQuery.bindValue(":tinyThumbnail", inByteArrayTiny);
    if (!insertThumbnailQuery.exec())
        qDebug() << "DB: Failed in insert Thubmanailfor " << astroFile.FullPath << insertThumbnailQuery.lastError();

    QSqlQuery tagStatusQuery;
    tagStatusQuery.prepare("UPDATE fits set ThumbnailStatus = :thumbnailStatus WHERE FullPath = :fullPath");
    tagStatusQuery.bindValue(":thumbnailStatus", astroFile.thumbnailStatus);
    tagStatusQuery.bindValue(":fullPath", astroFile.FullPath);
    if (!tagStatusQuery.exec())
        qDebug() << "FAILED to execute UPDATE Thumbnail Status query" << tagStatusQuery.lastError();
}

void FileRepository::getDuplicateFiles()
{
    getDuplicateFilesByFileHash();
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
    }
}

void FileRepository::loadThumbnail(const AstroFile &afi)
{
    if (cancelSignaled)
        return;
    QSqlQuery query;
    query.prepare("SELECT * FROM thumbnails where fits_id = :fitsId");
    query.bindValue(":fitsId", afi.Id);
    query.exec();

    int fits_idId = query.record().indexOf("fits_id");
    int idThumbnail = query.record().indexOf("thumbnail");
    int idTinyThumbnail = query.record().indexOf("tiny_thumbnail");

    AstroFile astroFile;
    int id = 0;
    if (query.first())
    {
        id = query.record().value(fits_idId).toInt();
        QByteArray inByteArray = query.value(idThumbnail).toByteArray();
        QImage image;
        image.loadFromData(inByteArray, "PNG");
        astroFile.thumbnail = image;
        astroFile.Id = afi.Id;

        QByteArray inByteArrayTiny = query.value(idTinyThumbnail).toByteArray();
        QImage imageTiny;
        imageTiny.loadFromData(inByteArrayTiny, "PNG");
        astroFile.tinyThumbnail = imageTiny;
        astroFile.Id = afi.Id;
    }
    emit thumbnailLoaded(astroFile);
}

void FileRepository::loadTagStats(const QString fileExtension, QList<QPair<QString, QString>>& filters)
{
    //.open astrocat.db
    // select tagKey,tagValue,count(*) from tags where tagKey IN ('OBJECT','INSTRUME','FILTER','FILEEXT') group by tagKey,tagValue;

    // select FileExtension,count(*) from fits group by FileExtension;

    // select FileExtension,count(*) from fits INNER JOIN tags on tags.fits_id=fits.Id group by FileExtension;

    // from fits join tags on fits.id = tags.fits_id where tagKey IN ('OBJECT','INSTRUME','FILTER','FILEEXT') and fits.fileextension = 'xisf' group by tagKey,tagValue;

    // select fileextension,tagKey,tagValue,count(*) from fits join tags on fits.id = tags.fits_id where tagKey IN ('OBJECT','INSTRUME','FILTER','FILEEXT') group by fileextension,tagKey,tagValue;

    // select fileextension,tagKey,tagValue,count(*) from fits join tags on fits.id = tags.fits_id where tagKey = 'OBJECT' AND tagValue = 'M81' group by fileextension,tagKey,tagValue;

    // select fileextension,tagKey,tagValue,count(*) from fits join tags on fits.id = tags.fits_id where fits.id in
    // (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = 'OBJECT' AND tagValue = 'M81')
    // and tagKey IN ('OBJECT','INSTRUME','FILTER','FILEEXT') group by fileextension,tagKey,tagValue;


    // select fileextension,tagKey,tagValue,count(*) from fits join tags on fits.id = tags.fits_id
    // where fits.id in
    //  ( select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = 'FILTER' AND tagValue = 'Ha')
    // AND fits.id in (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = 'OBJECT' AND tagValue = 'M81')
    // group by fileextension,tagKey,tagValue
    // HAVING tagkey in ('OBJECT','INSTRUME','FILTER','FILEEXT');

    // select fileextension,tagKey,tagValue,count(*) from fits join tags on fits.id = tags.fits_id
    // where fits.id in
    //  ( select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = 'FILTER' AND tagValue = 'Ha')
    // AND fits.id in (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = 'OBJECT' AND tagValue = 'M81')
    // group by fileextension,tagKey,tagValue
    // HAVING tagkey in ('OBJECT','INSTRUME','FILTER','FILEEXT');

    QStringList andStatements;
    QString andStatement;

    if (filters.count() > 0)
    {
        for (auto& a : filters)
        {
            QString statement = "fits.id in (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = '" + a.first + "' AND tagValue = '" + a.second + "') ";
            andStatements << statement;
        }
        andStatement = "where " + andStatements.join(" AND ");
    }
    if (!fileExtension.isEmpty())
    {
        andStatement = andStatement + " AND fits.fileExtension = '" + fileExtension + "' ";
    }

    QString queryString = "SELECT tagKey,tagValue,count(*) CNT FROM fits join tags on fits.id = tags.fits_id " +
                        andStatement +
            "group by tagKey,tagValue "
            "HAVING tagkey in ('OBJECT','INSTRUME','FILTER'); ";

    qDebug() <<queryString;

    QSqlQuery query(db);
    query.prepare(queryString);
//    query.exec();

    query.exec();
//    int idFileExtension = query.record().indexOf("fileextension");
    int idTagKey = query.record().indexOf("tagKey");
    int idTagValue = query.record().indexOf("tagValue");
    int idCount = query.record().indexOf("CNT");

//    qDebug()<<"=========== TAGS ==============";
    QList<FilterViewGroupData> groupData;
    while (query.next())
    {
//        QString fileExtension = query.value(idFileExtension).toString();
        QString tagKey = query.value(idTagKey).toString();
        QString tagValue = query.value(idTagValue).toString();
        int count = query.value(idCount).toInt();

        FilterViewGroupData data;
        data.GroupName = tagKey;
        data.ElementName = tagValue;
        data.ElementCount = count;
        groupData<<data;
//        qDebug()<<tagKey<<"|"<<tagValue<<"|"<<count;
    }
//    qDebug()<<"-------------------------";

    emit filterStatsLoaded(groupData);
}

void FileRepository::loadFileExtensionStats(const QString fileExtension, QList<QPair<QString, QString>>& filters)
{
    QStringList andStatements;
    QString andStatement;
    QStringList havingStatements;
    QString havingStatement;
    QString joinStatement;

    if (!fileExtension.isEmpty())
    {
        andStatements<< " fits.fileextension = '" + fileExtension + "' ";
    }

    if (filters.count() > 0)
    {
        for (auto& a : filters)
        {
            QString statement = "fits.id in (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = '" + a.first + "' AND tagValue = '" + a.second + "') ";
            andStatements << statement;
            havingStatements << "'" + a.second +"'";
        }
        joinStatement = "join tags on fits.id = tags.fits_id ";
        havingStatement = ",tagValue HAVING tagValue in (" + havingStatements.join(",") + ")";
    }
    if (!andStatements.isEmpty())
    {
        andStatement = " where " + andStatements.join(" AND ") ;
    }

    QString queryString = "SELECT fileextension,count(*) CNT FROM fits " +
                        joinStatement + andStatement + " group by fileextension " + havingStatement + ";";

    qDebug() <<queryString;

    QSqlQuery query(db);
    query.prepare(queryString);
//    query.exec();

    query.exec();
    int idFileExtension = query.record().indexOf("fileextension");
    int idCount = query.record().indexOf("CNT");

//    qDebug()<<"========== FILE EXTENSIONS ===============";
    QMap<QString, int> extensions;
    while (query.next())
    {
        QString fileExtension = query.value(idFileExtension).toString();
        int count = query.value(idCount).toInt();

        extensions[fileExtension] = count;
//        qDebug()<<fileExtension<<"|"<<count;
    }
//    qDebug()<<"-------------------------";

    emit fileExtensionStatsLoaded(extensions);
}

void FileRepository::loadAstroFiles(const QString fileExtension, QList<QPair<QString, QString> > &filters)
{
    QStringList andStatements;
    QString andStatement;
    QString joinStatement;

    if (filters.count() > 0)
    {
        for (auto& a : filters)
        {
            QString statement = "fits.id in (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = '" + a.first + "' AND tagValue = '" + a.second + "') ";
            andStatements << statement;
        }
        andStatement = andStatements.join(" AND ");
        joinStatement = " join tags on fits.id = tags.fits_id ";
    }
    if (!fileExtension.isEmpty())
    {
        if (!andStatement.isEmpty())
            andStatement = andStatement + " AND ";
        andStatement = andStatement + " fits.fileExtension = '" + fileExtension + "' ";
    }
    if (!andStatement.isEmpty())
    {
        andStatement = " where " + andStatement;
    }

    QString queryString = "SELECT DISTINCT fits.id FROM fits " + joinStatement + andStatement + "; ";

    qDebug() <<queryString;

    QSqlQuery query(db);
    query.prepare(queryString);
//    query.exec();

    query.exec();
//    int idFileExtension = query.record().indexOf("fileextension");
    int idKey = query.record().indexOf("Id");

//    qDebug()<<"=========== TAGS ==============";
//    QList<int> idList;
    QSet<int> idSet;
    int rows = query.numRowsAffected();
    idSet.reserve(rows);
    while (query.next())
    {
        int id = query.value(idKey).toInt();

        idSet<<id;
    }
//    qDebug()<<"-------------------------";

    emit astroFilesInFilter(idSet);

//    QStringList andStatements;
//    QString andStatement;
//    QStringList havingStatements;
//    QString havingStatement;
//    QString joinStatement;

//    if (!fileExtension.isEmpty())
//    {
//        andStatements<< " fits.fileextension = '" + fileExtension + "' ";
//    }

//    if (filters.count() > 0)
//    {
//        for (auto& a : filters)
//        {
//            QString statement = "fits.id in (select fits.id from fits join tags on fits.id = tags.fits_id where tagKey = '" + a.first + "' AND tagValue = '" + a.second + "') ";
//            andStatements << statement;
//            havingStatements << "'" + a.second +"'";
//        }
//        joinStatement = "join tags on fits.id = tags.fits_id join thumbnails on fits.id = thumbnails.fits_id";
////        havingStatement = ",tagValue HAVING tagValue in (" + havingStatements.join(",") + ")";
//    }
//    if (!andStatements.isEmpty())
//    {
//        andStatement = " where " + andStatements.join(" AND ") ;
//    }

//    QString queryString = "SELECT * FROM fits " +
//                        joinStatement + andStatement + ";";

//    qDebug() <<queryString;

//    QSqlQuery query(db);
//    query.prepare(queryString);

//    query.exec();
//    int idFileExtension = query.record().indexOf("fileextension");
//    int idCount = query.record().indexOf("CNT");

//    qDebug()<<"========== FILEs ===============";
//    QMap<QString, int> extensions;
//    QMap<int, AstroFile> files;
//    while (query.next())
//    {
//        QString fileExtension = query.value(idFileExtension).toString();
//        int idId = query.record().indexOf("Id");
//        int idFileName = query.record().indexOf("FileName");
//        int idFullPath = query.record().indexOf("FullPath");
//        int idDirectoryPath = query.record().indexOf("DirectoryPath");
//        int idVolumeName = query.record().indexOf("VolumeName");
//        int idVolumeRoot = query.record().indexOf("VolumeRoot");
//        int idFileType = query.record().indexOf("FileType");
//        int idFileExtension = query.record().indexOf("FileExtension");
//        int idCreatedTime = query.record().indexOf("CreatedTime");
//        int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
//        int idFileHash = query.record().indexOf("FileHash");
//        int idImageHash = query.record().indexOf("ImageHash");
//        int idTagStatus = query.record().indexOf("TagStatus");
//        int idThumbnailStatus = query.record().indexOf("ThumbnailStatus");
//        int idProcessStatus = query.record().indexOf("ProcessStatus");
//        int idIsHidden = query.record().indexOf("IsHidden");

//        AstroFile astro;
//        int astroFileId = query.value(idId).toInt();
//        astro.Id = astroFileId;
//        astro.FileName = query.value(idFileName).toString();
//        astro.FullPath = query.value(idFullPath).toString();
//        astro.DirectoryPath = query.value(idDirectoryPath).toString();
//        astro.VolumeName = query.value(idVolumeName).toString();
//        astro.VolumeRoot = query.value(idVolumeRoot).toString();
//        astro.FileType = AstroFileType(query.value(idFileType).toInt());
//        astro.FileExtension = query.value(idFileExtension).toString();
//        astro.FileHash = query.value(idFileHash).toString();
//        astro.ImageHash = query.value(idImageHash).toString();
//        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
//        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
//        astro.thumbnailStatus = ThumbnailLoadStatus(query.value(idThumbnailStatus).toInt());
//        astro.tagStatus = TagExtractStatus(query.value(idTagStatus).toInt());
//        astro.processStatus = AstroFileProcessStatus(query.value(idProcessStatus).toInt());
//        astro.IsHidden = AstroFileProcessStatus(query.value(idIsHidden).toInt());

//        files.insert(astroFileId, astro);
//    }
//    qDebug()<<"-------------------------";
}

QMap<int, AstroFile> FileRepository::_getAllAstrofiles()
{
    QSqlQuery query("SELECT * FROM fits");
    query.exec();
    int idId = query.record().indexOf("Id");
    int idFileName = query.record().indexOf("FileName");
    int idFullPath = query.record().indexOf("FullPath");
    int idDirectoryPath = query.record().indexOf("DirectoryPath");
    int idVolumeName = query.record().indexOf("VolumeName");
    int idVolumeRoot = query.record().indexOf("VolumeRoot");
    int idFileType = query.record().indexOf("FileType");
    int idFileExtension = query.record().indexOf("FileExtension");
    int idCreatedTime = query.record().indexOf("CreatedTime");
    int idLastModifiedTime = query.record().indexOf("LastModifiedTime");
    int idFileHash = query.record().indexOf("FileHash");
    int idImageHash = query.record().indexOf("ImageHash");
    int idTagStatus = query.record().indexOf("TagStatus");
    int idThumbnailStatus = query.record().indexOf("ThumbnailStatus");
    int idProcessStatus = query.record().indexOf("ProcessStatus");
    int idIsHidden = query.record().indexOf("IsHidden");

    QMap<int, AstroFile> files;
    while (query.next())
    {
        AstroFile astro;
        int astroFileId = query.value(idId).toInt();
        astro.Id = astroFileId;
        astro.FileName = query.value(idFileName).toString();
        astro.FullPath = query.value(idFullPath).toString();
        astro.DirectoryPath = query.value(idDirectoryPath).toString();
        astro.VolumeName = query.value(idVolumeName).toString();
        astro.VolumeRoot = query.value(idVolumeRoot).toString();
        astro.FileType = AstroFileType(query.value(idFileType).toInt());
        astro.FileExtension = query.value(idFileExtension).toString();
        astro.FileHash = query.value(idFileHash).toString();
        astro.ImageHash = query.value(idImageHash).toString();
        astro.CreatedTime = query.value(idCreatedTime).toDateTime();
        astro.LastModifiedTime = query.value(idLastModifiedTime).toDateTime();
        astro.thumbnailStatus = ThumbnailLoadStatus(query.value(idThumbnailStatus).toInt());
        astro.tagStatus = TagExtractStatus(query.value(idTagStatus).toInt());
        astro.processStatus = AstroFileProcessStatus(query.value(idProcessStatus).toInt());
        astro.IsHidden = AstroFileProcessStatus(query.value(idIsHidden).toInt());

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
    QSqlQuery query("SELECT fits_id, tiny_thumbnail FROM thumbnails");
    query.exec();
    int fits_idId = query.record().indexOf("fits_id");
//    int idThumbnail = query.record().indexOf("thumbnail");
    int idThumbnailTiny = query.record().indexOf("tiny_thumbnail");

    QMap<int, QImage> images;
    while (query.next())
    {
        AstroFile astro;
        int fitsId = query.value(fits_idId).toInt();
//        QByteArray inByteArray = query.value(idThumbnail).toByteArray();
        QByteArray inByteArrayTiny = query.value(idThumbnailTiny).toByteArray();

        images[fitsId].loadFromData(inByteArrayTiny, "PNG");
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
    emit modelLoadingGotAstrofiles();
//    // 2. Get the entire tags table into memory
//    // select * from tags

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
//        fitsmap[fitsId].thumbnail = QImage(20, 20, QImage::Format::Format_RGB32);
    }
    emit modelLoadingGotTags();

    // 4. Get the entire thumbnails into memory
    // select * from thumbnails

//    auto thumbnails = _getAllThumbnails();
//    emit modelLoadingGotThumbnails();

    // 5. Add thumbnails to their fits files
    // insert all thumbnails from #4 into map by fits_id

//    QMapIterator<int, QImage> thumbiter(thumbnails);
//    while (thumbiter.hasNext())
//    {
//        thumbiter.next();
//        auto fitsId = thumbiter.key();
//        auto& image = thumbiter.value();
//        fitsmap[fitsId].tinyThumbnail = image;
//        fitsmap[fitsId].thumbnailStatus = ThumbnailLoaded;
//    }

    // 6. convert map's `values` into a list of astroFile, and emit the list

    auto list = fitsmap.values();
    emit modelLoaded(list);
}
