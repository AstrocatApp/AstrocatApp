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

#include "fileprocessor.h"
#include "imageprocessor.h"
#include "xisfprocessor.h"
#include "newfileprocessor.h"
#include "fitsprocessor.h"

NewFileProcessor::NewFileProcessor(QObject *parent) : QObject(parent)
{
}

void NewFileProcessor::processNewFile(const QFileInfo& fileInfo)
{
    if (cancelSignaled)
    {
        qDebug() << "Cancel signaled. Draining Fits Tag Queue.";
        emit processingCancelled(fileInfo);
        return;
    }

    AstroFile astroFile(fileInfo);
    AstroFileImage afi(astroFile, QImage());

    afi = extractTags(afi);
    afi = extractThumbnail(afi);
    afi.processStatus = Processed;
    emit astrofileProcessed(afi);

}

AstroFileImage NewFileProcessor::extractTags(const AstroFileImage &astroFileImage)
{
    FileProcessor* processor = getProcessorForFile(astroFileImage.astroFile);
    processor->extractTags(astroFileImage);
    auto tags = processor->getTags();
    AstroFileImage afi(astroFileImage);
    afi.astroFile.Tags.swap(tags);
    afi.tagStatus = TagExtracted;

    delete processor;

    return afi;
}

AstroFileImage NewFileProcessor::extractThumbnail(const AstroFileImage &astroFileImage)
{
    FileProcessor* processor = getProcessorForFile(astroFileImage.astroFile);
    processor->extractThumbnail(astroFileImage);
    auto thumbnail = processor->getThumbnail();
    AstroFileImage afi(astroFileImage);
    afi.image = thumbnail;
    afi.thumbnailStatus = Loaded;

    delete processor;

    return afi;
}

void NewFileProcessor::cancel()
{
    cancelSignaled = true;
}

FileProcessor* NewFileProcessor::getProcessorForFile(const QFileInfo &fileInfo)
{
    const AstroFile af(fileInfo);
    return getProcessorForFile(af);
}

FileProcessor* NewFileProcessor::getProcessorForFile(const AstroFile &astroFile)
{
    switch (astroFile.FileType)
    {
        case AstroFileType::Fits:
            return new FitsProcessor();
        case AstroFileType::Xisf:
            return new XisfProcessor();
        case AstroFileType::Image:
            return new ImageProcessor();
    }
}
