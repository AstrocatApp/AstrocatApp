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

#include <QElapsedTimer>
#include <QDebug>
#include <QtGlobal>
#include <vector>

/*
 * This Class is an implementation of the "Display Function" (Section 8.5.6) and
 * "Adaptive Display Function Algorithm" (Section 8.5.7),
 * as described in the Pixinsight XISF Specification
 *
 * https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html#__XISF_Data_Objects_:_XISF_Image_:_Display_Function__
 * https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html#__XISF_Data_Objects_:_XISF_Image_:_Adaptive_Display_Function_Algorithm__
 *
 * The initial implementation was experimental and mainly for learning purposes.
 * None of the routines have been optimized yet.
 *
 * This implementation has not been extensively tested yet. Only a handful of image
 * formats were used for testing.
 *
 * There are many performance optimizations possible. The DisplayFunction,
 * ExpansionFunction, ClippingFunction and the MidtonesTransferFunction were implemented
 * directly as described by the specification. Most of those can be reorganized and even
 * can be moved inside a single function, with pre-computed values. For example, the
 * ExpansionFunction is redundant when r=1 and l=0
 *
 * At this time, the median and medianf functions consume significant amount of time.
 * We'll need to combine them and maybe use sampling to reduce the amount of time spent
 * in these functions.
 *
 * We are normalizing the data before any further calculations, which is probably not
 * needed. The spec was mainly giving floating point examples and calculations in the
 * [0,1] range, and therefore we followed that practise. We porobably can get away without
 * normalizing the input.
 */

template<typename T>
AutoStretcher<T>::AutoStretcher(int width, int height, int numberOfChannels, int fitsDataType)
{
    _width = width;
    _height = height;
    _numberOfChannels = numberOfChannels;
    _fitsDataType = fitsDataType;
}

template<typename T>
AutoStretcher<T>::~AutoStretcher()
{
    delete [] _normalData;
}

template<typename T>
void AutoStretcher<T>::setData(T *data)
{
    _data = data;
    getRange();
    normalize();
}

template<typename T>
void AutoStretcher<T>::normalize()
{
    _normalData = new float[_width*_height*_numberOfChannels];
    T* it = _data;
    float* fIt = _normalData;
    float range = _rangeMax - _rangeMin;
    for (int c = 0; c < _numberOfChannels; c++)
    {
        for (int i = 0; i < _height; i++)
        {
            for (int j = 0; j < _width; j++)
            {
                    *fIt = (float)*it / range;
                    it++;
                    fIt++;
            }
        }
    }
}

template<typename T>
StretchParams AutoStretcher<T>::getParams()
{
    return stretchParams;
}

template <typename T>
float median(T* data, int len)
{
    std::vector<float> d;
    d.assign(data, data + len);
    std::sort(d.begin(), d.end());
    return d[len/2];
}

template <typename T>
float medianf(std::vector<T> &data)
{
    const int mid = data.size() / 2;
    std::nth_element(data.begin(), data.begin() + mid, data.end());
    return data[mid];
}

template <typename T>
void AutoStretcher<T>::calculateParams()
{
    QElapsedTimer timer;
    timer.start();

    int channelSize = _width * _height;
    float* channelMedians = new float[_numberOfChannels];
    for (int k = 0; k < _numberOfChannels; k++)
    {
        float* channelArr = &_normalData[k*channelSize];
//        T* channelArr = &_data[k*channelSize];
        float channelMedian = median(channelArr, channelSize);
        qDebug() << "median took" << timer.elapsed() << "milliseconds";
        channelMedians[k] = channelMedian;

        float* it = _normalData;
//        T* it = _data;
        std::vector<float> v;
        for (int i = 0; i < _height; i++)
        {
            for (int j = 0; j < _width; j++)
            {
                v.push_back(abs((*it) - channelMedian));
                it++;
            }
        }
        float med = medianf(v);
        qDebug() << "medianf took" << timer.elapsed() << "milliseconds";
        float normalizedMedian = 1.4826f * med;

        float B = 0.25f;
        float C = -2.8f;
        int A = channelMedian > 0.5 ? 1: 0;
        float S;
        if (A == 1)
            S = 0;
        else if (normalizedMedian == 0)
        {
            S = 0;
        }
        else
        {
            S = fmin(1, fmax(0, channelMedian + C * normalizedMedian));
        }

        float H;
        if (A == 0)
            H = 1;
        else if (normalizedMedian == 0)
        {
            H = 1;
        }
        else
        {
            H = fmin(1, fmax(0, channelMedian - C * normalizedMedian));
        }

        float M;
        if (A == 0)
        {
            M = MidtonesTransferFunction(channelMedian - S, B);
        }
        else
        {
            M = MidtonesTransferFunction(B, H - channelMedian);
        }
        stretchParams.channel[k] = {A, B, C, S, H, M};
        qDebug() << "Channel took" << timer.elapsed() << "milliseconds";
    }

    delete [] channelMedians;
}

template<typename T>
void AutoStretcher<T>::getRange()
{
    T* it = (T*)_data;
    _rangeMax = _rangeMin = *it;

    for (int i = 0; i < _height; i++)
    {
        for (int j = 0; j < _width; j++)
        {
            for (int k = 0; k < _numberOfChannels; k++)
            {
                T x = *it;
                if (x > _rangeMax)
                    _rangeMax = x;
                if (x < _rangeMin)
                    _rangeMin = x;
                it++;
            }
        }
    }
}

template<typename T>
void AutoStretcher<T>::stretch()
{
    float m = 0.5;
    float s = 0;
    float h = 1;
    float l = 0;
    float r = 1;

    stretch(m, s, h, l, r);
}

template<typename T>
void AutoStretcher<T>::stretch(float m, float s, float h, float l, float r)
{
    calculateParams();
    T* dit = (T*)_data;
    float* it = _normalData;
    float range = _rangeMax - _rangeMin;
    Q_ASSERT(range != 0);

    for (int k = 0; k < _numberOfChannels; k++)
    {
        for (int i = 0; i < _height; i++)
        {
            for (int j = 0; j < _width; j++)
            {
//                float x = (float)*it / range;
                float x = *it;

                auto sp = stretchParams.channel[k];
                m = sp.M;
                s = sp.S;
                h = sp.H;
                l = 0;
                r = 1;
                int xxxx= 0;
                float stretched = DisplayFunction(x, m, s, h, l, r);
                if (stretched > 0 )
                    xxxx++;
                float stretchedAndRanged = stretched*255; // 255 is not universal
                T xxx = (T)stretchedAndRanged;
                *dit = xxx;
                dit++;
                it++;
            }
        }
    }
}

template<typename T>
float AutoStretcher<T>::MidtonesTransferFunction(float x, float m)
{
    if (x == 0.0f)
    {
        return 0;
    }

    if (x == 1.0f)
    {
        return 1;
    }

    if (x == m)
    {
        return 0.5f;
    }

    Q_ASSERT((2 * m - 1) * x - m != 0);
    return (m - 1) * x / ( (2 * m - 1) * x - m);
}

template<typename T>
float AutoStretcher<T>::ClippingFunction(float x, float s, float h)
{
    if (x < s)
    {
        return 0;
    }

    if (x > h)
    {
        return 1;
    }

    Q_ASSERT(h != s);
    return (x - s) / (h - s);
}

template<typename T>
float AutoStretcher<T>::ExpansionFunction(float x, float l, float r)
{
    Q_ASSERT(r != l);
    return (x - l) / (r - l);
}

template<typename T>
float AutoStretcher<T>::DisplayFunction(float x, float m, float s, float h, float l, float r)
{
    /*
     * Identity Function is for values
     * m = 1/2
     * s = 0
     * h = 1
     * l = 0
     * r = 1
     */
    return ExpansionFunction(MidtonesTransferFunction(ClippingFunction(x, s, h), m), l, r);
}


template class AutoStretcher<int8_t>;
template class AutoStretcher<int16_t>;
template class AutoStretcher<int32_t>;
template class AutoStretcher<int64_t>;
template class AutoStretcher<float>;
template class AutoStretcher<double>;
template class AutoStretcher<uint16_t>;
template class AutoStretcher<uint32_t>;
