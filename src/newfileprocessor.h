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

#ifndef NEWFILEPROCESSOR_H
#define NEWFILEPROCESSOR_H

#include "astrofile.h"
#include "catalog.h"
#include "fileprocessor.h"

#include <QFileInfo>
#include <QObject>

class NewFileProcessor : public QObject
{
    Q_OBJECT
public:
    explicit NewFileProcessor(QObject *parent = nullptr);
    virtual void setCatalog(Catalog* cat);
    virtual void processNewFile(const QFileInfo& fileInfo);
    virtual void cancel();

signals:
    void astrofileProcessed(const AstroFile& astroFile);
    void processingCancelled(const QFileInfo& fileInfo);

protected:
    volatile bool cancelSignaled = false;
    Catalog* catalog;

private:
    FileProcessor* getProcessorForFile(const QFileInfo& fileInfo);
    FileProcessor* getProcessorForFile(const AstroFile& astroFile);

    QByteArray getFileHash(const QFileInfo& fileInfo);
};

#endif // NEWFILEPROCESSOR_H
