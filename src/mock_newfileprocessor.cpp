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

#include "mock_newfileprocessor.h"

#include <QPainter>
#include <QThread>

QImage makeImage(int num, bool isTiny)
{
    int size = isTiny ? 20 : 200;

    QImage image(QSize(size,size),QImage::Format_RGB32);
    QPainter painter(&image);
    painter.setBrush(QBrush(Qt::green));
    painter.fillRect(QRectF(0,0,size,size),Qt::green);
    painter.fillRect(QRectF(size/4,size/4,size/2,size/2),Qt::white);
    painter.setPen(QPen(Qt::black));
    painter.drawText(QRect(size/4,size/4,size/2,size/2), isTiny ? "T" : "F" + QString::number(num));
    return image;
}

void Mock_NewFileProcessor::processNewFile(const QFileInfo &fileInfo)
{
    if (cancelSignaled)
        return;

    // Let's put some back pressure. If we emit too fast, the Db won't be able
    // to catch up with the writes. Although it is a good test, it is not very
    // realistic, as processing files will almost always take longer than writing
    // them to the DB. Although if due to any reason the DB can't write fast enough
    // then we will consume huge amounts of memory due to piling up emits with large
    // thumbnails in them. (Ex: if we implement parallel file processing, and the
    // files are on a fast disk, but the DB is on a slow or busy disk).
    QThread::msleep(50);

    static int lastId = 1;
    QImage tiny = makeImage(lastId, true);
    QImage thum = makeImage(lastId, false);

    AstroFile astroFile(fileInfo);
    astroFile.processStatus = AstroFileProcessed;
    astroFile.Tags.insert({{"OBJECT", "value1"}, {"INSTRUME", "value2"}});
    astroFile.tagStatus = TagExtracted;
    astroFile.thumbnail = thum;
    astroFile.tinyThumbnail = tiny;
    astroFile.ImageHash = "hash" + QString::number(lastId);

    lastId++;
    emit astrofileProcessed(astroFile);
}

void Mock_NewFileProcessor::setCatalog(Catalog *cat)
{
    this->catalog = cat;
}
