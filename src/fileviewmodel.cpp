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

#include <QIcon>
#include <QMimeData>
#include <QPixmap>
#include <QPixmapCache>
#include <QUrl>

FileViewModel::FileViewModel(QObject* parent) : QAbstractItemModel(parent)
{
    rc = 0;
    cc = 1;
}

FileViewModel::~FileViewModel()
{
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

    if (row < rc  && row >= 0 && column < cc && column >= 0)
    {
        auto a = catalog->getAstroFile(row);
        return createIndex(row, 0, a);
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

    beginRemoveRows(parent, row, row + count - 1);
    rc-= count;
    endRemoveRows();
    if (rc == 0)
        emit modelIsEmpty(true);

    // Do not "emit" astroFileDeleted, but instead call it directly.
    // Otherwise due to threading, the row will be wrong.
    catalog->deleteAstroFileRow(row);
    return true;
}

bool FileViewModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_UNUSED(count);

    beginInsertColumns(parent,column, column + count -1);
    cc += count;
    endInsertColumns();
    return true;
}

bool FileViewModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;
    return true;
}

void FileViewModel::setCatalog(Catalog *cat)
{
    this->catalog = cat;
}

void FileViewModel::setCellSize(const int newSize)
{
    emit layoutAboutToBeChanged();
    int size = 400 * newSize/100;
    cellSize = QSize(size,size);
    emit layoutChanged();
}

QString FileViewModel::raConverter(QString ra) const
{
    return ra;
}

QString FileViewModel::decConverter(QString dec) const
{
    bool isNeg = false;
    bool ok;
    float fDec = dec.toFloat(&ok);
    if (!ok)
        return QString();

    if (fDec < 0)
    {
        isNeg = true;
        fDec = -fDec;
    }

    int degrees = (int) fDec;
    fDec -= degrees;

    fDec *= 60;
    int arcMins = (int) fDec;

    fDec -= arcMins;
    fDec *= 60;
    int arcSecs = (int) fDec;

    fDec -= arcSecs;
    fDec *= 10;
    int arcSecDecimal = (int) fDec;

    QString decString = QString{ "%1%2:%3:%4.%5" }.arg( isNeg ? '-' : '+' ).arg(degrees,2).arg(arcMins,2).arg(arcSecs,2).arg(arcSecDecimal);
    return decString;
}

QVariant FileViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row() >= rc)
    {
        return QVariant();
    }
    auto astroFile = catalog->getAstroFile(index.row());
    if (astroFile == nullptr)
        return QVariant();

    switch (role)
    {
        case Qt::DisplayRole:
        {
            return astroFile->FileName;
        }
        case Qt::DecorationRole:
        {
            QPixmap pixmap;
            if (!QPixmapCache::find(QString::number(astroFile->Id), &pixmap))
            {
                emit loadThumbnailFromDb(*astroFile);
                pixmap = QPixmap::fromImage(astroFile->tinyThumbnail).scaled( cellSize*0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            QIcon icon;
//            for (auto state : {QIcon::Off, QIcon::On}){
//                   for (auto mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected})
//                       icon.addPixmap(pixmap, mode, state);
//            }
            icon.addPixmap(pixmap, QIcon::Normal);
            icon.addPixmap(pixmap, QIcon::Selected);
            return icon;
        }
        case AstroFileRoles::ItemRole:
        {
            auto af = catalog->getAstroFile(index.row());
//            AstroFile a(*af);
            return QVariant::fromValue<AstroFile*>(af);
        }
        case Qt::SizeHintRole:
        {
            return cellSize;
        }
        case AstroFileRoles::IdRole:
        {
            return astroFile->Id;
        }
        case AstroFileRoles::ObjectRole:
        {
            return astroFile->Tags["OBJECT"];
        }
        case AstroFileRoles::InstrumentRole:
        {
            return astroFile->Tags["INSTRUME"];
        }
        case AstroFileRoles::FilterRole:
        {
            return astroFile->Tags["FILTER"];
        }
        case AstroFileRoles::DateRole:
        {
            return astroFile->Tags["DATE-OBS"];
        }
        case AstroFileRoles::FullPathRole:
        {
            return astroFile->FullPath;
        }
        case AstroFileRoles::DirectoryRole:
        {
            return astroFile->DirectoryPath;
        }
        case AstroFileRoles::VolumeNameRole:
        {
            return astroFile->VolumeName;
        }
        case AstroFileRoles::VolumeRootRole:
        {
            return astroFile->VolumeRoot;
        }
        case AstroFileRoles::RaRole:
        {
            return astroFile->Tags["OBJCTRA"];
        }
        case AstroFileRoles::DecRole:
        {
            return astroFile->Tags["OBJCTDEC"];
        }
        case AstroFileRoles::CcdTempRole:
        {
            return astroFile->Tags["CCD-TEMP"];
        }
        case AstroFileRoles::ImageXSizeRole:
        {
            return astroFile->Tags["NAXIS1"];
        }
        case AstroFileRoles::ImageYSizeRole:
        {
            return astroFile->Tags["NAXIS2"];
        }
        case AstroFileRoles::GainRole:
        {
            return astroFile->Tags["GAIN"];
        }
        case AstroFileRoles::ExposureRole:
        {
            return astroFile->Tags["EXPTIME"];
        }
        case AstroFileRoles::BayerModeRole:
        {
            return astroFile->Tags["BAYERPAT"];
        }
        case AstroFileRoles::OffsetRole:
        {
            return astroFile->Tags["BLKLEVEL"];
        }
        case AstroFileRoles::CalframeRole:
        {
            return astroFile->Tags["CALFRAME"];
        }
        case AstroFileRoles::FileTypeRole:
        {
            return astroFile->FileType;
        }
        case AstroFileRoles::FileExtensionRole:
        {
            return astroFile->FileExtension;
        }
        case AstroFileRoles::FileHashRole:
        {
            return astroFile->FileHash;
        }
    }

    return QVariant();
}

void FileViewModel::AddAstroFiles(int numberAdded)
{
    insertRows(rc, numberAdded, QModelIndex());
}

void FileViewModel::UpdateAstroFile(AstroFile astroFile, int row)
{
    Q_UNUSED(astroFile);

    auto index = createIndex(row, 0);
    emit dataChanged(index, index);
}

void FileViewModel::RemoveAstroFile(const AstroFile& astroFile)
{
    int row = catalog->astroFileIndex(astroFile);
    removeRow(row);
}

void FileViewModel::RemoveAstroFiles(const QList<AstroFile> &astroFiles)
{
    if (astroFiles.isEmpty())
        return;
    int row = catalog->astroFileIndex(astroFiles.first());
    removeRows(row, astroFiles.count(), QModelIndex());
}

void FileViewModel::addThumbnail(const AstroFile &astroFile)
{
    int row = catalog->astroFileIndex(astroFile);
    auto index = createIndex(row, 0);

    QPixmapCache::insert(QString::number(astroFile.Id), QPixmap::fromImage(astroFile.thumbnail.scaled( QSize(400,400), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    emit dataChanged(index, index, {Qt::DecorationRole});
}

bool FileViewModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);

    emit fileDropped(data->urls());

    return true;
}

Qt::DropActions FileViewModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags FileViewModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

     if (index.isValid())
         return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
     else
         return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList FileViewModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

QMimeData *FileViewModel::mimeData(const QModelIndexList &indexes) const
{
    const AstroFile* astroFile = static_cast<AstroFile*>(indexes[0].internalPointer());

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(QList<QUrl>() << QUrl::fromLocalFile(astroFile->FullPath));
    return mimeData;
}
