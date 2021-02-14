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

#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include "astrofile.h"

#include <QDate>
#include <QObject>
#include <QSortFilterProxyModel>

class SortFilterProxyModel : public  QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortFilterProxyModel(QObject *parent = nullptr);
    QDate filterMinimumDate() const { return minDate; }

    QDate filterMaximumDate() const { return maxDate; }

public slots:
    void setFilterMinimumDate(QDate date);
    void setFilterMaximumDate(QDate date);
    void addAcceptedFilter(QString filterName);
    void removeAcceptedFilter(QString filterName);
    void addAcceptedInstrument(QString instrumentName);
    void removeAcceptedInstrument(QString instrumentName);
    void addAcceptedObject(QString objectName);
    void removeAcceptedObject(QString objectName);

signals:
    void filterMinimumDateChanged(QDate date) const;
    void filterMaximumDateChanged(QDate date) const;
    void astroFileAccepted(const AstroFile& astroFile) const;
    void filterReset() const;

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
    bool dateInRange(QDate date) const;
    bool instrumentAccepted(QString instrument) const;
    bool objectAccepted(QString object) const;
    bool filterAccepted(QString filter) const;
    QDate minDate;
    QDate maxDate;

    QList<QString> acceptedFilters;
    QList<QString> acceptedObjects;
    QList<QString> acceptedInstruments;
};

#endif // SORTFILTERPROXYMODEL_H
