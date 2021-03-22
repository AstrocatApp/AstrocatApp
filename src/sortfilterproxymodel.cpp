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

SortFilterProxyModel::SortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    const AstroFileImage* astroFileImage = static_cast<AstroFileImage*>(index.internalPointer());
    Q_ASSERT(astroFileImage != NULL);

    QString dateString = astroFileImage->astroFile.Tags["DATE-OBS"];
    QDate d = QDate::fromString(dateString, Qt::ISODate);

    bool shouldAccept = dateInRange(d) && objectAccepted(astroFileImage->astroFile.Tags["OBJECT"]) && instrumentAccepted(astroFileImage->astroFile.Tags["INSTRUME"]) && filterAccepted(astroFileImage->astroFile.Tags["FILTER"]);
//    bool shouldAccept = dateInRange(d) && objectAccepted(astroFile->Tags.value("OBJECT")) && instrumentAccepted(astroFile->Tags.value("INSTRUME")) && filterAccepted(astroFile->Tags.value("FILTER"));

    if (shouldAccept && shouldAcceptTagsForFilters(astroFileImage))
    {
        emit astroFileAccepted(astroFileImage->astroFile);
    }

    return shouldAccept;
}

bool SortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    // TODO: Sorting logic should be implemented here.
    Q_UNUSED(source_left);
    Q_UNUSED(source_right);

    return true;
}

bool SortFilterProxyModel::dateInRange(QDate date) const
{
    return (!minDate.isValid() || date >= minDate) &&
           (!maxDate.isValid() || date <= maxDate);
}

bool SortFilterProxyModel::shouldAcceptTagsForFilters(const AstroFileImage* astroFileImage) const
{
    return astroFileImage->tagStatus == TagExtracted;
}

bool SortFilterProxyModel::instrumentAccepted(QString instrument) const
{
    return acceptedInstruments.empty() || acceptedInstruments.contains(instrument) || (acceptedInstruments.contains("None") && instrument.isEmpty());
}

bool SortFilterProxyModel::objectAccepted(QString object) const
{
    return acceptedObjects.empty() || acceptedObjects.contains(object) || (acceptedObjects.contains("None") && object.isEmpty());
}

bool SortFilterProxyModel::filterAccepted(QString filter) const
{
    return acceptedFilters.empty() || acceptedFilters.contains(filter) || (acceptedFilters.contains("None") && filter.isEmpty());
}

void SortFilterProxyModel::setFilterMinimumDate(QDate date)
{
    minDate = date;
    emit filterReset();
    invalidateFilter();
}

void SortFilterProxyModel::setFilterMaximumDate(QDate date)
{
    maxDate = date;
    emit filterReset();
    invalidateFilter();
}

void SortFilterProxyModel::addAcceptedFilter(QString filterName)
{
    if (!acceptedFilters.contains(filterName))
    {
        acceptedFilters.append(filterName);
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedFilter(QString filterName)
{
    if (acceptedFilters.removeOne(filterName))
        invalidateFilter();
}

void SortFilterProxyModel::addAcceptedInstrument(QString instrumentName)
{
    if (!acceptedInstruments.contains(instrumentName))
    {
        acceptedInstruments.append(instrumentName);
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedInstrument(QString instrumentName)
{
    if (acceptedInstruments.removeOne(instrumentName))
        invalidateFilter();
}

void SortFilterProxyModel::addAcceptedObject(QString objectName)
{
    if (!acceptedObjects.contains(objectName))
    {
        acceptedObjects.append(objectName);
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedObject(QString objectName)
{
    if (acceptedObjects.removeOne(objectName))
        invalidateFilter();
}
