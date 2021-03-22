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

FileViewModel::FileViewModel(QObject* parent) : QAbstractItemModel(parent)
{
    rc = 0;
    cc = 1;
}

FileViewModel::~FileViewModel()
{
}

void FileViewModel::setInitialModel(const QList<AstroFileImage> &files)
{
    int count = 0;

    for (auto& i : files)
    {
        fileList.append(i);
        fileMap.insert(i.astroFile.FullPath, i);
        insertRow(count);
        count++;
    }
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
        //return fileList[row]->index;
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
    return true;
}

bool FileViewModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;
    beginRemoveRows(parent, row, row + count -1);
    rc-= count;
    auto astroFileImage = fileList.at(row);
    auto& f = fileMap[astroFileImage.astroFile.FullPath];
    fileMap.remove(astroFileImage.astroFile.FullPath);
    fileList.removeAt(row);
    endRemoveRows();
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
    return fileMap.contains(fullPath);
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
    int index = 0;
    for (auto& af : fileList)
    {
        if (af.astroFile.FullPath == astroFile.FullPath)
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
            return a.astroFile.FileName;
        }
        case Qt::DecorationRole:
        {
            if (a.image.isNull())
            {
                auto img = QImage(":Icons/resources/loading.png");
                return img;
            }
            QImage small = a.image.scaled( cellSize*0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return small;
        }
        case Qt::SizeHintRole:
        {
            return cellSize;
        }
        case AstroFileRoles::ObjectRole:
        {
            return a.astroFile.Tags["OBJECT"];
        }
        case AstroFileRoles::InstrumentRole:
        {
            return a.astroFile.Tags["INSTRUME"];
        }
        case AstroFileRoles::FilterRole:
        {
            return a.astroFile.Tags["FILTER"];
        }
        case AstroFileRoles::DateRole:
        {
            return a.astroFile.Tags["DATE-OBS"];
        }
        case AstroFileRoles::FullPathRole:
        {
            return a.astroFile.FullPath;
        }
        case AstroFileRoles::RaRole:
        {
            return a.astroFile.Tags["RA"];
        }
        case AstroFileRoles::DecRole:
        {
            return a.astroFile.Tags["DEC"];
        }
        case AstroFileRoles::CcdTempRole:
        {
            return a.astroFile.Tags["CCD-TEMP"];
        }
        case AstroFileRoles::ImageXSizeRole:
        {
            return a.astroFile.Tags["NAXIS1"];
        }
        case AstroFileRoles::ImageYSizeRole:
        {
            return a.astroFile.Tags["NAXIS2"];
        }
        case AstroFileRoles::GainRole:
        {
            return a.astroFile.Tags["GAIN"];
        }
        case AstroFileRoles::ExposureRole:
        {
            return a.astroFile.Tags["EXPTIME"];
        }
        case AstroFileRoles::BayerModeRole:
        {
            return a.astroFile.Tags["BAYERPAT"];
        }
        case AstroFileRoles::OffsetRole:
        {
            return a.astroFile.Tags["BLKLEVEL"];
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
    fileMap.clear();

    fileList.clear();
    rc=0;
    emit endResetModel();
}

void FileViewModel::addTags(const AstroFileImage &astroFileImage)
{
    fileMap[astroFileImage.astroFile.FullPath].astroFile.Tags.clear();
    fileMap[astroFileImage.astroFile.FullPath].astroFile.Tags.insert(astroFileImage.astroFile.Tags);
    fileMap[astroFileImage.astroFile.FullPath].tagStatus = astroFileImage.tagStatus;

    QModelIndex index = getIndexForAstroFile(astroFileImage.astroFile);
    fileList[index.row()].astroFile.Tags.clear();
    fileList[index.row()].astroFile.Tags.insert(astroFileImage.astroFile.Tags);
    fileList[index.row()].tagStatus = astroFileImage.tagStatus;

    emit dataChanged(index, index);
}

void FileViewModel::addThumbnail(const AstroFileImage &astroFileImage)
{
    fileMap[astroFileImage.astroFile.FullPath].image = astroFileImage.image;
    fileMap[astroFileImage.astroFile.FullPath].thumbnailStatus = astroFileImage.thumbnailStatus;

    QModelIndex index = getIndexForAstroFile(astroFileImage.astroFile);
    fileList[index.row()].image = astroFileImage.image;
    fileList[index.row()].thumbnailStatus = astroFileImage.thumbnailStatus;

    emit dataChanged(index, index);
}

void FileViewModel::addAstroFile(const AstroFileImage &astroFileImage)
{
    bool afiExists = fileMap.contains(astroFileImage.astroFile.FullPath);
    if (!afiExists)
    {
        // This is a new file
        fileList.append(astroFileImage);
        fileMap[astroFileImage.astroFile.FullPath] = astroFileImage;
        insertRow(rc);
    }
    else
    {
        QModelIndex index = getIndexForAstroFile(astroFileImage.astroFile);
        fileList[index.row()] = astroFileImage;
        fileMap[astroFileImage.astroFile.FullPath] = astroFileImage;
        emit dataChanged(index, index);
    }
}
