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

#ifndef AUTOSTRETCHER_H
#define AUTOSTRETCHER_H

struct StretchParam
{
    int A;
    float B;
    float C;
    float S;
    float H;
    float M;
};

struct StretchParams
{
    StretchParam channel[3];
};

template <typename T>
class AutoStretcher
{
public:
    AutoStretcher(int width, int height, int numberOfChannels, int fitsDataType);
    ~AutoStretcher();
    void setData(T* data);
    void stretch();
    void calculateParams();
    void normalize();
    StretchParams getParams();
private:
    int _width;
    int _height;
    int _numberOfChannels;
    int _fitsDataType;
    T _rangeMax;
    T _rangeMin;
    T* _data;
    float* _normalData;
    StretchParams stretchParams;

    float MidtonesTransferFunction(float x, float m);
    float ClippingFunction(float x, float s, float h);
    float ExpansionFunction(float x, float l, float r);

    float DisplayFunction(float x, float m, float s, float h, float l, float r);
    void getRange();
};

#endif // AUTOSTRETCHER_H
