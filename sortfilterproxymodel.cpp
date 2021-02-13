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

#include "sortfilterproxymodel.h"
#include "fileviewmodel.h"

#include <QDate>

QDate SortFilterProxyModel::dateRangeMin;
QDate SortFilterProxyModel::dateRangeMax;

SortFilterProxyModel::SortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    AstroFile* astroFile = static_cast<AstroFile*>(index.internalPointer());
    QString dateString = astroFile->Tags["DATE-OBS"].remove("'");
    QDate d = QDate::fromString(dateString, Qt::ISODate);

//    auto data = sourceModel()->data(index, AstroFileRoles::DateRole).toString();
//    data.remove("'");

//    auto d = QDate::fromString(data, Qt::ISODate);

    bool shouldAccept = dateInRange(d);

    if (shouldAccept)
    {
        emit astroFileAccepted(*astroFile);
//        if (!dateRangeMin.isValid() ||  d < dateRangeMin)
//        {
//            dateRangeMin = d;
//            emit FilterMinimumDateChanged(d);

//        }
//        if (!dateRangeMax.isValid() ||  d > dateRangeMax)
//        {
//            dateRangeMax = d;
//            emit FilterMaximumDateChanged(d);
//        }
    }

    return shouldAccept;
}

bool SortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    return true;
}

bool SortFilterProxyModel::dateInRange(QDate date) const
{
    return (!minDate.isValid() || date >= minDate)
            && (!maxDate.isValid() || date <= maxDate);
}

void SortFilterProxyModel::setFilterMinimumDate(QDate date)
{
    qDebug() << "SortFilterProxyModel::setFilterMinimumDate";
    minDate = date;
    emit filterReset();
    invalidateFilter();
}

void SortFilterProxyModel::setFilterMaximumDate(QDate date)
{
    qDebug() << "SortFilterProxyModel::setFilterMaximumDate";
    maxDate = date;
    emit filterReset();
    invalidateFilter();
}
