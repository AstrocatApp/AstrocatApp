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
    myparent = parent;
}

FileViewModel::~FileViewModel()
{
    for (auto& a : fileList)
        delete a;
}

void FileViewModel::SetInitialAstrofiles(const QList<AstroFile> &files)
{
    int count = 0;

    for (auto& i : files)
    {
        AstroFileImage* afi = new AstroFileImage(i, QImage());
        fileList.append(afi);
        fileMap.insert(i.FullPath, afi);
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
    // none of our items have valid parents, because they all are root items
    return QModelIndex();
}

QModelIndex FileViewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < fileList.count()  && row >= 0 && column < cc && column >= 0)
    {
        //return fileList[row]->index;
        return createIndex(row, 0, fileList[row]);
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
    auto& f = fileMap[astroFileImage->astroFile.FullPath];
    fileMap.remove(astroFileImage->astroFile.FullPath);
    fileList.removeOne(f);
    delete f;

    endRemoveRows();
    return true;
}

bool FileViewModel::insertColumns(int column, int count, const QModelIndex &parent)
{
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

bool FileViewModel::AstroFileExists(const QString fullPath)
{
    return fileMap.contains(fullPath);
}

void FileViewModel::setCellSize(const int newSize)
{
    int size = 400 * newSize/100;
    cellSize = QSize(size,size);
    emit layoutChanged();
}

int FileViewModel::getRowForAstroFile(const AstroFile& astroFile)
{
    int index = 0;
    for (auto& af : fileList)
    {
        if (af->astroFile.FullPath == astroFile.FullPath)
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

void FileViewModel::GetThumbnailFinished(const AstroFile &astroFile, const QPixmap &pixmap)
{
    QImage img;
    if (pixmap.isNull())
    {
        // Put a default image if the image cannot be processed
        bool ret = img.load(":Icons/resources/nopreview.png");
        if (!ret)
            qDebug() << "Failed to load nopreview.png";
    }
    else
        img = pixmap.toImage();

    fileMap[astroFile.FullPath]->image = img;

    // Emit that the data has changed here.
    QModelIndex index = getIndexForAstroFile(astroFile);
    emit dataChanged(index, index);
}

QVariant FileViewModel::data(const QModelIndex &index, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
        {
            if (index.row() >= fileList.count())
            {
                qDebug()<<"Invalid Index";
                return QVariant();
            }
            auto a = fileList.at(index.row());
            return a->astroFile.FileName;
        }
        case Qt::DecorationRole:
        {
            if (index.row() >= fileList.count())
            {
                qDebug()<<"Invalid Index";
                return QVariant();
            }
            auto a = fileList.at(index.row());
            if (a->image.isNull())
            {
                emit GetThumbnail(a->astroFile.FullPath);
                auto img = QImage(":Icons/resources/loading.png");
                return img;
            }
            QImage small = a->image.scaled( cellSize*0.9, Qt::KeepAspectRatio);
            return small;
        }
        case Qt::SizeHintRole:
        {
            return cellSize;
        }
        case AstroFileRoles::ObjectRole:
        {
            auto a = fileList.at(index.row());
            return a->astroFile.Tags["OBJECT"];
        }
        case AstroFileRoles::InstrumentRole:
        {
            auto a = fileList.at(index.row());
            return a->astroFile.Tags["INSTRUME"];
        }
        case AstroFileRoles::FilterRole:
        {
            auto a = fileList.at(index.row());
            return a->astroFile.Tags["FILTER"];
        }
        case AstroFileRoles::DateRole:
        {
            auto a = fileList.at(index.row());
            return a->astroFile.Tags["DATE-OBS"];
        }
    }

    return QVariant();
}

// This should insert or update the AstroFile
void FileViewModel::AddAstroFile(AstroFile astroFile, const QImage& image)
{
    if (AstroFileExists(astroFile.FullPath))
    {
        auto& f = fileMap[astroFile.FullPath];
        f->astroFile = astroFile;
        f->image = image;
        QModelIndex index = getIndexForAstroFile(astroFile);
        emit dataChanged(index, index);
    }
    else
    {
        AstroFileImage* afi = new AstroFileImage(astroFile, image);
        fileList.append(afi);
        fileMap.insert(astroFile.FullPath, afi);
        insertRow(rc);
    }
}

void FileViewModel::RemoveAstroFile(AstroFile astroFile)
{
    int row = getRowForAstroFile(astroFile);
    if (row>=0)
    {
        removeRow(row);
    }
}
