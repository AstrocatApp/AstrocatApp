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

#ifndef MOCK_NEWFILEPROCESSOR_H
#define MOCK_NEWFILEPROCESSOR_H

#include "newfileprocessor.h"

#include <QObject>

class Mock_NewFileProcessor : public NewFileProcessor
{
    Q_OBJECT
public:

signals:

public:
    void processNewFile(const QFileInfo &fileInfo);

    // NewFileProcessor interface
public:
    void setCatalog(Catalog *cat);

private:
    QList<QString> Objects = {"M81", "M101", "Hearth", "IC1805", "IC410", "M31", "M33"};
    QList<QString> Filters = {"Lum", "Red", "Green", "Blue", "Ha", "SII", "OIII"};
    QList<QString> Instruments = {"QHY294M", "QHY600M", "QHY268M", "QHY268C", "SVBONY", "Canon Ra"};
};

#endif // MOCK_NEWFILEPROCESSOR_H
