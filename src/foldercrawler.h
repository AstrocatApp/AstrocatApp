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

#ifndef FOLDERCRAWLER_H
#define FOLDERCRAWLER_H

#include <QFileInfo>
#include <QObject>
#include <QUrl>
#include <QWaitCondition>

class FolderCrawler : public QObject
{
    Q_OBJECT
public:
    explicit FolderCrawler(QObject *parent = nullptr);
    ~FolderCrawler();

public slots:
    virtual void crawl(QString rootFolder);
    virtual void crawlUrl(QUrl rootFolder);
    virtual void cancel();
    virtual void pause();
    virtual void resume();

signals:
    void fileFound(QFileInfo filePath);
    void startedCrawlingFolder(QString folder);
    void endededCrawlingFolder(QString folder);

protected:
    volatile bool cancelSignaled = false;
    volatile bool pauseSignaled = false;
    QMutex pauseMutex;
    QWaitCondition pauseCondition;;
};

#endif // FOLDERCRAWLER_H
