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

#include "fitsfile.h"

#include <QCryptographicHash>

FitsFile::FitsFile()
{
    _fptr = 0;
}

FitsFile::~FitsFile()
{
    int status = 0;
    fits_close_file(_fptr, &status);
}

#define CHK_STATUS(status) { if (status)  \
{ \
    char err_text[1024]; \
    fits_get_errstatus( status, err_text); \
    qDebug() << err_text; \
return; \
} }

bool FitsFile::loadFile(QString filePath)
{
    int status = 0;
    fits_open_file(&_fptr, filePath.toStdString().c_str(), READONLY, &status);

    return status == 0;
}

void FitsFile::extractTags()
{
    int hdupos, nkeys;
    char card[FLEN_CARD];
    int status = 0;

    fits_get_hdu_num(_fptr, &hdupos);

    while (!status)
    {
        hdupos++;
        fits_get_hdrspace(_fptr, &nkeys, NULL, &status);

        for (int i = 1; i <= nkeys; i++)
        {
           if (fits_read_record(_fptr, i, card, &status))
               break;

           char keyname[FLEN_KEYWORD];
           char keyvalue[FLEN_VALUE];
           char comment[FLEN_COMMENT];

           fits_read_keyn(_fptr,i, keyname, keyvalue, comment, &status);
           _tags.insert(QString(keyname).remove("'").trimmed(), QString(keyvalue).remove("'").trimmed());
        }

        fits_movrel_hdu(_fptr, 1, NULL, &status);
    }
}

QByteArray calculateHash(QByteArray &array)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    return hash.hash(array,QCryptographicHash::Sha1);
}

void FitsFile::extractImage()
{
    int status = 0;
    int imageType;
    _qImageFormat = QImage::Format::Format_Invalid;

    fits_get_img_type(_fptr, &imageType, &status);
    CHK_STATUS(status);

    fits_get_img_equivtype(_fptr, &_imageEquivType, &status);
    CHK_STATUS(status);

    int fitsDataType = 0;
    long long naxesLongLongArr[3] = {0,0,0};
    int bitpix, naxis;

    fits_get_img_paramll(_fptr, 2, &bitpix, &naxis, naxesLongLongArr, &status);

    if (naxis < 2)
    {
        // Not a 2D Image
        return;
    }

    _numberOfChannels = _tags.contains("BAYERPAT") || _tags.value("NAXIS3") == "3"? 3: 1;

    _width = naxesLongLongArr[0];
    _height = naxesLongLongArr[1];
    long long numberOfPixels = _width * _height;
    _bytesPerPixel = 0;

    switch (_imageEquivType)
    {
    case BYTE_IMG:
        fitsDataType = TBYTE;
        _bytesPerPixel = sizeof(int8_t);
        break;
    case SHORT_IMG:
        fitsDataType = TUSHORT;
        _bytesPerPixel = sizeof(int16_t);
        break;
    case LONG_IMG:
        fitsDataType = TULONG;
        _bytesPerPixel = sizeof(int32_t);
        break;
    case LONGLONG_IMG:
        fitsDataType = TLONGLONG;
        _bytesPerPixel = sizeof(int64_t);
        break;
    case FLOAT_IMG:
        fitsDataType = TFLOAT;
        _bytesPerPixel = sizeof(float);
        break;
    case DOUBLE_IMG:
        fitsDataType = TDOUBLE;
        _bytesPerPixel = sizeof(double);
        break;
    case SBYTE_IMG:
        fitsDataType = TBYTE;
        _bytesPerPixel = sizeof(int8_t);
        break;
    case USHORT_IMG:
        fitsDataType = TUSHORT;
        _bytesPerPixel = sizeof(uint16_t);
        break;
    case ULONG_IMG:
        fitsDataType = TULONG;
        _bytesPerPixel = sizeof(uint32_t);
        break;
    case ULONGLONG_IMG:
        fitsDataType = TULONGLONG;
        _bytesPerPixel = sizeof(int64_t);
        break;
    }

    _qImageFormat = _numberOfChannels == 3 ? QImage::Format::Format_RGB32 : QImage::Format::Format_Grayscale8;

    _data = new unsigned char[numberOfPixels * _bytesPerPixel * _numberOfChannels];
    fits_movabs_hdu(_fptr, 1, IMAGE_HDU, &status);
    CHK_STATUS(status);
    int m_FITSBITPIX, ndim;
    long naxes[3];
    fits_get_img_param(_fptr, 3, &m_FITSBITPIX, &ndim, naxes, &status);
    CHK_STATUS(status);

    fits_read_img(_fptr, fitsDataType, 1, numberOfPixels, NULL, _data, NULL, &status);
    CHK_STATUS(status);

    auto hashData = QByteArray((char*)_data, numberOfPixels * _bytesPerPixel * _numberOfChannels);
    _imageHash = calculateHash(hashData);

    if (_numberOfChannels == 3)
    switch (_imageEquivType)
    {
    case BYTE_IMG:
        deBayer<int8_t>();
        break;
    case SHORT_IMG:
        deBayer<int16_t>();
        break;
    case LONG_IMG:
        deBayer<int32_t>();
        break;
    case LONGLONG_IMG:
        deBayer<int64_t>();
        break;
    case FLOAT_IMG:
        deBayer<float>();
        break;
    case DOUBLE_IMG:
        deBayer<double>();
        break;
    case SBYTE_IMG:
        deBayer<int8_t>();
        break;
    case USHORT_IMG:
        deBayer<uint16_t>();
        break;
    case ULONG_IMG:
        deBayer<uint32_t>();
        break;
    case ULONGLONG_IMG:
        deBayer<int64_t>();
        break;
    }

    switch (_imageEquivType)
    {
        case BYTE_IMG:
        {
            AutoStretcher<int8_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((int8_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<int8_t>();
            break;
        }
        case SHORT_IMG:
        {
            AutoStretcher<int16_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((int16_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<int16_t>();
            break;
        }
        case LONG_IMG:
        {
            AutoStretcher<int32_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((int32_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<int32_t>();
            break;
        }
        case LONGLONG_IMG:
        {
            AutoStretcher<int64_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((int64_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<int64_t>();
            break;
        }
        case FLOAT_IMG:
        {
            AutoStretcher<float> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((float*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<float>();
            break;
        }
        case DOUBLE_IMG:
        {
            AutoStretcher<double> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((double*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<double>();
            break;
        }
        case SBYTE_IMG:
        {
            AutoStretcher<int8_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((int8_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<int8_t>();
            break;
        }
        case USHORT_IMG:
        {
            AutoStretcher<uint16_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((uint16_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<uint16_t>();
            break;
        }
        case ULONG_IMG:
        {
            AutoStretcher<uint32_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((uint32_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<uint32_t>();
            break;
        }
        case ULONGLONG_IMG:
        {
            AutoStretcher<int64_t> as(_width, _height, _numberOfChannels, fitsDataType);
            as.setData((int64_t*)_data);
            as.calculateParams();
            as.stretch();
            makeImage<int64_t>();;
            break;
        }
    }

    delete [] _data;
}

template <typename T>
void FitsFile::makeImage()
{
    QImage out(_width, _height, _qImageFormat);
    long size = _width * _height;
    T* it = (T*)_data;

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
    this->_qImage = out;
    return;
}

template <typename T>
void FitsFile::deBayer()
{
    T * data = reinterpret_cast<T*>(_data);
    T* dataRed = new T[_width * _height/4];
    T* dataGreen = new T[_width * _height/4];
    T* dataBlue = new T[_width * _height/4];

    T* redIt = dataRed;
    T* greenIt = dataGreen;
    T* blueIt = dataBlue;

    for (int i = 0; (i+1) < _height; i+=2)
    {
        for (int j = 0; (j+1) < _width; j+=2)
        {
            T _red    = (data[i*_width + j]);
            T _green1 = (data[i*_width + j+1]);
            T _green2 = (data[(i+1)*_width + j]);
            T _blue   = (data[(i+1)*_width + j+1]);

            *redIt   = _red;
            *greenIt = (_green1 + _green2)/2;
            *blueIt = _blue;
            redIt++;
            greenIt++;
            blueIt++;
        }
    }
    _width/=2;
    _height/=2;
    delete [] _data;
    _data = (unsigned char*)new T[_width * _height * 3];
    long size = _width * _height * sizeof(T);
    memcpy (_data + 0, dataRed, size);
    memcpy (_data + size, dataGreen, size);
    memcpy (_data + 2*size, dataBlue, size);
    delete [] dataRed;
    delete [] dataGreen;
    delete [] dataBlue;
}
