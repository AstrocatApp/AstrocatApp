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

#ifndef ASTROFILE_H
#define ASTROFILE_H

#include <QDateTime>
#include <QFileInfo>
#include <QString>
#include <qimage.h>

enum ThumbnailLoadStatus
{
    Loaded,
    NotProcessedYet,
    FailedToProess
};

enum TagExtractStatus
{
    TagExtracted,
    TagNotProcessedYet,
    TagFailedToProess
};

enum AstroFileType
{
    Fits,
    Xisf,
    Image
};

struct AstroFile
{
    QString FileName;
    QString FullPath;
    QString DirectoryPath;
    AstroFileType FileType;
    QDateTime CreatedTime;
    QDateTime LastModifiedTime;
    QMap<QString, QString> Tags;

    AstroFile()
    {
    }

    AstroFile(const QFileInfo& fileInfo)
    {
        FullPath = fileInfo.absoluteFilePath();
        CreatedTime = fileInfo.birthTime();
        LastModifiedTime = fileInfo.lastModified();
        DirectoryPath = fileInfo.canonicalPath();
        FileName = fileInfo.baseName();

        QString suffix = fileInfo.suffix().toLower();
        if ( suffix == "fits")
            FileType = AstroFileType::Fits;
        else if (suffix == "fit")
            FileType = AstroFileType::Fits;
        else if (suffix== "xisf")
            FileType = AstroFileType::Xisf;
        else if (suffix == "png")
            FileType = AstroFileType::Image;
        else if (suffix == "jpg")
            FileType = AstroFileType::Image;
        else if (suffix == "jpeg")
            FileType = AstroFileType::Image;
        else if (suffix == "tiff")
            FileType = AstroFileType::Image;
    }
};

struct AstroFileImage
{
    AstroFileImage() {}
    AstroFileImage(const AstroFileImage& other)
        :astroFile(other.astroFile), image(other.image), thumbnailStatus(other.thumbnailStatus), tagStatus(other.tagStatus)
    {}
    AstroFileImage(AstroFile file, QImage img, ThumbnailLoadStatus thumbnailStatus = ThumbnailLoadStatus::NotProcessedYet, TagExtractStatus tagStatus = TagExtractStatus::TagNotProcessedYet)
        :astroFile(file), image(img), thumbnailStatus(thumbnailStatus), tagStatus(tagStatus)
    {
    }
    AstroFile astroFile;
    QImage image;
    ThumbnailLoadStatus thumbnailStatus;
    TagExtractStatus tagStatus;
};

#endif // ASTROFILE_H
