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
        afi->index = createIndex(rc, 0, afi);
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
        return fileList[row]->index;
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

void FileViewModel::GetThumbnailFinished(const AstroFile &astroFile, const QPixmap &pixmap)
{
    qDebug() << "Got a THUMB";
    QImage img;
    if (pixmap.isNull())
    {
        // Put a default image if the image cannot be processed
        bool ret = img.load(":Pictures/resources/nopreview.png");
        if (!ret)
            qDebug() << "Failed to load nopreview.png";
    }
    else
        img = pixmap.toImage();
    fileMap[astroFile.FullPath]->image = img;

    //    Emit that the data has changed here.
    emit dataChanged(fileMap[astroFile.FullPath]->index, fileMap[astroFile.FullPath]->index);
}

QVariant FileViewModel::data(const QModelIndex &index, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
        {
            auto a = fileList.at(index.row());
            return a->astroFile.FileName;
        }
        case Qt::DecorationRole:
        {
            qDebug() << "DecorationRole called for " << index.row() << ":" << index.column();
            auto a = fileList.at(index.row());
            if (a->image.isNull())
            {
                qDebug()<<"NULL IMAGE";
                emit GetThumbnail(a->astroFile.FullPath);
                auto img = QImage(":Pictures/resources/loading.png");
                return img;
            }
            QImage small = a->image.scaled( cellSize*0.9, Qt::KeepAspectRatio);
            return small;
        }
        case Qt::SizeHintRole:
        {
            return cellSize;
        }
    }

    return QVariant();
}

// This should insert or update the AstroFile
void FileViewModel::AddAstroFile(AstroFile astroFile, const QImage& image)
{
    qDebug()<< "Adding file to view: " << astroFile.FileName;

    if (AstroFileExists(astroFile.FullPath))
    {
        auto& f = fileMap[astroFile.FullPath];
        f->astroFile = astroFile;
        f->image = image;
        emit dataChanged(f->index, f->index);
    }
    else
    {
        AstroFileImage* afi = new AstroFileImage(astroFile, image);
        afi->index = createIndex(rc, 0, afi);
        fileList.append(afi);
        fileMap.insert(astroFile.FullPath, afi);
        insertRow(rc);
    }
}
