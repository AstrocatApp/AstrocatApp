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

#include "autostretcher.h"
#include "xisfprocessor.h"

#include <QCryptographicHash>


using namespace pcl;

XisfProcessor::~XisfProcessor() noexcept
{
    xisf.Close();
}

bool XisfProcessor::loadFile(const AstroFile &astroFile)
{
    try
    {
        xisf.Open(astroFile.FullPath.toStdWString().c_str());
    }
    catch (pcl::Error)
    {
        return false;
    }
    return true;
}

QImage makeImage(int _width, int _height, float* _data, QImage::Format _qImageFormat, int _numberOfChannels)
{
    QImage out(_width, _height, _qImageFormat);
    long size = _width * _height;
    float* it = _data;

    if (_numberOfChannels == 3)
    {
        for (int i = 0; i < _height; i++)
        {
            auto * scanLine = reinterpret_cast<QRgb*>(out.scanLine(i));
            for (int j = 0; j < _width; j++)
            {
                int red = it[0];
                int green = it[size];
                int blue = it[2*size];
                scanLine[j] = qRgb(red, green, blue);
                it++;
            }
        }
    }
    else if (_numberOfChannels == 1)
    {
        for (int i = 0; i < _height; i++)
        {
            auto * scanLine = out.scanLine(i);
            for (int j = 0; j < _width; j++)
            {
                scanLine[j] = (*it);
                it++;
            }
        }
    }
    else
        Q_ASSERT(false);
    return out;
}

void XisfProcessor::extractTags()
{    
    auto fitsTags = xisf.ReadFITSKeywords();
    for (auto& f : fitsTags)
    {
        _tags.insert(QString(f.name.c_str()).remove("'").trimmed(), QString(f.value.c_str()).remove("'").trimmed());
    }
}

QByteArray XisfProcessor::calculateHash(QByteArray &array)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    return hash.hash(array,QCryptographicHash::Sha1);
}

void XisfProcessor::extractThumbnail()
{
    FImage image;
    xisf.ReadImage(image);

    int channels = image.NumberOfChannels();

    int height = image.Height();
    int width = image.Width();

    QImage::Format format = channels == 3 ? QImage::Format_RGB32 : QImage::Format_Grayscale8;

    float* data = new float[width*height*channels];
    float* it = data;

    for (int c = 0; c < channels; c++)
    {
        for (int h = 0; h < height; h++)
        {
            auto fscanLine = image.ScanLine(h, c);
            for (int w = 0; w < width; w++)
            {
                *it = fscanLine[w];
                it++;
            }
        }
    }

    auto hashData = QByteArray((char*)data, width * height * channels * sizeof(float));
    _imageHash = calculateHash(hashData);

    AutoStretcher<float> as(width, height, channels, 0);
    as.setData(data);
    as.calculateParams();
    as.stretch();
    QImage qimage = makeImage(width, height, data, format, channels);

    this->_thumbnail = qimage.scaled( QSize(200, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    delete [] data;
}

QByteArray XisfProcessor::getImageHash()
{
    return _imageHash;
}

QMap<QString, QString> XisfProcessor::getTags()
{
    return _tags;
}

QImage XisfProcessor::getThumbnail()
{
    return _thumbnail;
}
