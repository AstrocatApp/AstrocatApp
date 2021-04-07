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

#include "fileviewmodel.h"

#include <QPixmap>
#include <QPixmapCache>

FileViewModel::FileViewModel(QObject* parent) : QAbstractItemModel(parent)
{
    rc = 0;
    cc = 1;
}

FileViewModel::~FileViewModel()
{
}

void FileViewModel::setInitialModel(const QList<AstroFile> &files)
{
    int count = 0;

    for (auto& i : files)
    {
        fileList.append(i);
        filePathToIdMap.insert(i.FullPath, i.Id);
        fileIdMap[i.Id] = i;
        count++;
    }
    qDebug()<<"Done setting initial model";
    insertRows(0, count, QModelIndex());
    qDebug()<<"Done inserting roles";
}

int FileViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return rc;
}

int FileViewModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return cc;
}

QModelIndex FileViewModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    // none of our items have valid parents, because they all are root items
    return QModelIndex();
}

QModelIndex FileViewModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row < fileList.count()  && row >= 0 && column < cc && column >= 0)
    {
        return createIndex(row, 0, &fileList[row]);
    }
    return QModelIndex();
}

bool FileViewModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;
    beginInsertRows(parent, row, row + count -1);
    rc+= count;
    endInsertRows();
    if (rc >= 1)
        emit modelIsEmpty(false);

    return true;
}

bool FileViewModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    beginRemoveRows(parent, row, row);
    rc-= count;
    auto astroFile = fileList.at(row);    
    auto astroFileId = fileList.at(row);
    filePathToIdMap.remove(astroFile.FullPath);
    fileIdMap.remove(astroFile.Id);
    fileList.removeAt(row);
    endRemoveRows();
    if (rc == 0)
        emit modelIsEmpty(true);
    return true;
}

bool FileViewModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_UNUSED(count);
    beginInsertColumns(parent,column, column);
    cc++;
    endInsertColumns();
    return true;
}

bool FileViewModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;
    return true;
}

bool FileViewModel::astroFileExists(const QString fullPath)
{
    return filePathToIdMap.contains(fullPath);
}

AstroFile FileViewModel::getAstroFileById(int id)
{
    return fileIdMap[id];
}

int FileViewModel::getAstroFileIdByPath(const QString &fullPath)
{
    return filePathToIdMap[fullPath];
}

void FileViewModel::setCellSize(const int newSize)
{
    emit layoutAboutToBeChanged();
    int size = 400 * newSize/100;
    cellSize = QSize(size,size);
    emit layoutChanged();
}

int FileViewModel::getRowForAstroFile(const AstroFile& astroFile)
{
    Q_ASSERT(astroFile.Id != 0);
    int index = 0;
    for (auto& af : fileList)
    {
        if (af.Id == astroFile.Id)
        {
            return index;
        }
        index++;
    }
    return -1;
}

QModelIndex FileViewModel::getIndexForAstroFile(const AstroFile& astroFile)
{
    int row = getRowForAstroFile(astroFile);
    if (row == -1)
        return QModelIndex();

    return createIndex(row, 0, &astroFile); // we probably should not pass the astrofile pointer to this index
}

QVariant FileViewModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= fileList.count())
    {
        return QVariant();
    }
    auto a = fileList.at(index.row());

    switch (role)
    {
        case Qt::DisplayRole:
        {
            return a.FileName;
        }
        case Qt::DecorationRole:
        {
            QPixmap pixmap;
            if (!QPixmapCache::find(QString::number(a.Id), &pixmap))
            {
                emit loadThumbnailFromDb(a);
                auto img = a.tinyThumbnail.scaled( cellSize*0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                return img;
            }
            else
            {
                QImage image = pixmap.toImage();
                QImage small = image.scaled( cellSize*0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                return small;
            }
        }
        case Qt::SizeHintRole:
        {
            return cellSize;
        }
        case AstroFileRoles::IdRole:
        {
            return a.Id;
        }
        case AstroFileRoles::ObjectRole:
        {
            return a.Tags["OBJECT"];
        }
        case AstroFileRoles::InstrumentRole:
        {
            return a.Tags["INSTRUME"];
        }
        case AstroFileRoles::FilterRole:
        {
            return a.Tags["FILTER"];
        }
        case AstroFileRoles::DateRole:
        {
            return a.Tags["DATE-OBS"];
        }
        case AstroFileRoles::FullPathRole:
        {
            return a.FullPath;
        }
        case AstroFileRoles::RaRole:
        {
            return a.Tags["RA"];
        }
        case AstroFileRoles::DecRole:
        {
            return a.Tags["DEC"];
        }
        case AstroFileRoles::CcdTempRole:
        {
            return a.Tags["CCD-TEMP"];
        }
        case AstroFileRoles::ImageXSizeRole:
        {
            return a.Tags["NAXIS1"];
        }
        case AstroFileRoles::ImageYSizeRole:
        {
            return a.Tags["NAXIS2"];
        }
        case AstroFileRoles::GainRole:
        {
            return a.Tags["GAIN"];
        }
        case AstroFileRoles::ExposureRole:
        {
            return a.Tags["EXPTIME"];
        }
        case AstroFileRoles::BayerModeRole:
        {
            return a.Tags["BAYERPAT"];
        }
        case AstroFileRoles::OffsetRole:
        {
            return a.Tags["BLKLEVEL"];
        }
        case AstroFileRoles::FileTypeRole:
        {
            return a.FileType;
        }
        case AstroFileRoles::FileExtensionRole:
        {
            return a.FileExtension;
        }
        case AstroFileRoles::FileHashRole:
        {
            return a.FileHash;
        }
    }

    return QVariant();
}

void FileViewModel::removeAstroFile(AstroFile astroFile)
{
    int row = getRowForAstroFile(astroFile);
    if (row>=0)
    {
        removeRow(row);
    }
}

void FileViewModel::clearModel()
{
    emit beginResetModel();
    filePathToIdMap.clear();
    fileIdMap.clear();

    fileList.clear();
    rc=0;
    emit endResetModel();
}

void FileViewModel::addAstroFile(const AstroFile &astroFile)
{
    bool afiExists = filePathToIdMap.contains(astroFile.FullPath);
    if (!afiExists)
    {
        // This is a new file
        fileList.append(astroFile);
        filePathToIdMap[astroFile.FullPath] = astroFile.Id;
//        fileIdMap[astroFile.Id] = astroFile;
        fileIdMap.insert(astroFile.Id, astroFile);
        insertRow(rc);
    }
    else
    {
        QModelIndex index = getIndexForAstroFile(astroFile);
        fileList[index.row()] = astroFile;
        filePathToIdMap[astroFile.FullPath] = astroFile.Id;
        fileIdMap[astroFile.Id] = astroFile;
        emit dataChanged(index, index);
    }
}

void FileViewModel::addThumbnail(const AstroFile &astroFile)
{
    QModelIndex index = getIndexForAstroFile(astroFile);
    QPixmapCache::insert(QString::number(astroFile.Id), QPixmap::fromImage(astroFile.thumbnail));
    emit dataChanged(index, index, {Qt::DecorationRole});
}
