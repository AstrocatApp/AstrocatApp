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

#ifndef FITSFILE_H
#define FITSFILE_H

#include <QObject>
#include <QImage>
#include "astrofile.h"
#include "fitsio.h"

enum BayerPattern
{
    None, // Mono Image
    RGGB,
    Unsupported
};

enum ImageDataType
{
    BYTEIMG,
    SHORTIMG,
    LONGIMG,
    LONGLONGIMG,
    FLOATIMG,
    DOUBLEIMG
};

class FitsFile
{
public:
    FitsFile();
    ~FitsFile();

    bool loadFile(QString filaPath);
    int getNumberOfChannels()
    {
        return _numberOfChannels;
    }

    BayerPattern getBayerPattern()
    {
        return _bayerPattern;
    }

    QImage getImage()
    {
        return _qImage;
    }

    QMap<QString, QString> getTags()
    {
        return _tags;
    }

    void extractTags();
    void extractImage();

private:
    int _numberOfChannels;
    BayerPattern _bayerPattern;
    ImageDataType _imageDataType;
    QImage _qImage;
    QImage::Format _qImageFormat;
    QMap<QString, QString> _tags;
    fitsfile* _fptr;
    unsigned char* _data;
    int _imageEquivType;

    long long _width;
    long long _height;
    int _bytesPerPixel;
    template <typename T>
    void deBayer();
    template <typename T>
    void makeImage();
};

#endif // FITSFILE_H
