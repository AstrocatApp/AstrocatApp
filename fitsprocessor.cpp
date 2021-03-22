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

#include "fitsprocessor.h"
#include "fitsio.h"
#include "fitsfile.h"

QImage makeThumbnail(const QImage &image)
{
    QImage small =image.scaled( QSize(200, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return small;
}

void FitsProcessor::extractTags(const AstroFileImage &astroFileImage)
{
    FitsFile fits;
    auto ret = fits.loadFile(astroFileImage.astroFile.FullPath);
    if (!ret)
        // TODO: Return an astroimage with failed extraction status
        return;
    fits.extractTags();
    _tags = fits.getTags();
}

void FitsProcessor::extractThumbnail(const AstroFileImage &astroFileImage)
{
    FitsFile fits;
    auto ret = fits.loadFile(astroFileImage.astroFile.FullPath);
    if (!ret)
        // TODO: Return an astroimage with failed extraction status
        return;
    fits.extractImage();
    auto image = fits.getImage();
    _thumbnail = makeThumbnail(image);
}

