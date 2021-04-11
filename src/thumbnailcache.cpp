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

#include "thumbnailcache.h"

#include <QThread>
#include <QWaitCondition>

#define MAX_REQUEST 5

QStack<int> requests;
QWaitCondition bufferNotEmpty;
QMutex mutex;
volatile bool isCanceled = false;


void ThumbnailCache::run()
{
    while (!isCanceled)
    {
        mutex.lock();
        if (requests.isEmpty())
            bufferNotEmpty.wait(&mutex);
        mutex.unlock();

        if (isCanceled)
            break;

        mutex.lock();
        int id = requests.pop();
        AstroFile a;
        a.Id = id;
        emit dbLoadThumbnail(a);
        mutex.unlock();
    }

}

ThumbnailCache::ThumbnailCache(QObject *parent) : QThread(parent)
{

}

void ThumbnailCache::cancel()
{
    isCanceled = true;
    mutex.lock();
    bufferNotEmpty.wakeAll();
    mutex.unlock();
}

void ThumbnailCache::enqueueLoadThumbnail(const AstroFile &astroFile)
{
    mutex.lock();
    int requestsSize = requests.size();
    if (requests.contains(astroFile.Id))
    {
        mutex.unlock();
        return;
    }
    if (requestsSize >= MAX_REQUEST)
    {
        requests.removeLast();
        qDebug()<<"Dropping one request";
    }
    requests.push(astroFile.Id);
    bufferNotEmpty.wakeAll();
    mutex.unlock();
}
