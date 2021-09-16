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

#include "foldercrawler.h"

#include <QDirIterator>

FolderCrawler::FolderCrawler(QObject *parent) : QObject(parent)
{

}

FolderCrawler::~FolderCrawler()
{
}

void FolderCrawler::cancel()
{
    qDebug()<<"FolderCrawler::cancel()";
    cancelSignaled = true;
}

void FolderCrawler::pause()
{
    pauseMutex.lock();
    pauseSignaled = true;
    pauseMutex.unlock();
}

void FolderCrawler::resume()
{
    pauseMutex.lock();
    pauseSignaled = false;
    pauseMutex.unlock();
    pauseCondition.wakeAll();
    qDebug()<<"Woke up crawler thread";
}

void FolderCrawler::crawl(QString rootFolder)
{
    qDebug()<<"In FolderCrawler::crawl";
    QStringList extensions = {"*.fits", "*.fit", "*.xisf", "*.jpg", "*.jpeg", "*.png", "*.gif", "*.tif", "*.tiff", "*.bmp"};

    emit startedCrawlingFolder(rootFolder);
    QDirIterator it(rootFolder, extensions, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        if (cancelSignaled)
            return;
        pauseMutex.lock();
        if (pauseSignaled)
        {
            qDebug()<<"crawler sleeping";
            pauseCondition.wait(&pauseMutex);
            qDebug()<<"crawler woke up";
        }
        pauseMutex.unlock();
        it.next();
        emit fileFound(it.fileInfo());
    }
    emit endededCrawlingFolder(rootFolder);
    qDebug() << "Done crawling... " << rootFolder;
}

void FolderCrawler::crawlUrl(QUrl rootFolder)
{
    this->crawl(rootFolder.path());
}
