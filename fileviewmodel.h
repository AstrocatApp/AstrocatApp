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

#ifndef FILEVIEWMODEL_H
#define FILEVIEWMODEL_H

#include "astrofile.h"

#include <QAbstractItemModel>
#include <QImage>

class FileViewModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    FileViewModel(QObject* parent = nullptr);
    ~FileViewModel();
    void SetInitialAstrofiles(const QList<AstroFile>& files);
    void AddAstroFile(AstroFile astroFile, const QImage& image);

    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const  override;
    bool insertRows(int row, int count, const QModelIndex &parent)  override;
    bool insertColumns(int column, int count, const QModelIndex &parent)  override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    bool hasChildren(const QModelIndex &parent) const override;
    bool AstroFileExists(const QString fullPath);

public slots:
    void setCellSize(const int newSize);
    void GetThumbnailFinished(const AstroFile& astroFile, const QPixmap& pixmap);

signals:
    void GetThumbnail(const QString fullPath) const;

private:
    struct AstroFileImage
    {
        AstroFileImage(AstroFile file, QImage img) :astroFile(file), image(img)
        {
        }
        AstroFile astroFile;
        QImage image;
        QModelIndex index;
    };
    QList<AstroFileImage*> fileList;
    QMap<QString, AstroFileImage*> fileMap;

    int rc;
    int cc;
    QObject* myparent;
    QSize cellSize = QSize(200, 200);

};

#endif // FILEVIEWMODEL_H