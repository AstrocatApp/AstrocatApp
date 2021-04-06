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

#include "imageprocessor.h"

#include <QCryptographicHash>

QByteArray ImageProcessor::fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile sourceFile(fileName);
    qint64 fileSize = sourceFile.size();
    const qint64 bufferSize = 10240;

    if (sourceFile.open(QIODevice::ReadOnly))
    {
        char buffer[bufferSize];
        int bytesRead;
        int readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash(hashAlgorithm);
        while (readSize > 0 && (bytesRead = sourceFile.read(buffer, readSize)) > 0)
        {
            fileSize -= bytesRead;
            hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);
        }

        sourceFile.close();
        return hash.result();
    }
    return QByteArray();
}

bool ImageProcessor::loadFile(const AstroFile &astroFile)
{
    if (!image.load(astroFile.FullPath))
    {
        return false;
    }
    _imageHash = fileChecksum(astroFile.FullPath, QCryptographicHash::Sha1);
    return true;
}

void ImageProcessor::extractTags()
{
}

void ImageProcessor::extractThumbnail()
{
    _thumbnail = image.scaled( QSize(200, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QMap<QString, QString> ImageProcessor::getTags()
{
    return _tags;
}

QImage ImageProcessor::getThumbnail()
{
    return _thumbnail;
}

QByteArray ImageProcessor::getImageHash()
{
    return _imageHash;
}

