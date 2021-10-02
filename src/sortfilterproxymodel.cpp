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
    isDuplicatedFilterActive = false;
}

bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    const AstroFile* astroFile = static_cast<AstroFile*>(index.internalPointer());
    Q_ASSERT(astroFile != NULL);

//    QString dateString = astroFile->Tags["DATE-OBS"];
//    QDate d = QDate::fromString(dateString, Qt::ISODate);

//    bool shouldAccept = dateInRange(d) && objectAccepted(astroFile->Tags["OBJECT"]) && instrumentAccepted(astroFile->Tags["INSTRUME"]) && filterAccepted(astroFile->Tags["FILTER"]) && extensionAccepted(astroFile->FileExtension) && folderAccepted(astroFile->VolumeName, astroFile->DirectoryPath);
    bool shouldAccept = filterIsActive ? acceptedAstroFilesId.contains(astroFile->Id) : true;

    if (isDuplicatedFilterActive)
        shouldAccept = shouldAccept && isDuplicateOf(astroFile->FileHash);
    return shouldAccept;
}

bool SortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    // TODO: Sorting logic should be implemented here.
    Q_UNUSED(source_left);
    Q_UNUSED(source_right);

    return true;
}

bool SortFilterProxyModel::dateInRange(const QDate& date) const
{
    return (!minDate.isValid() || date >= minDate) &&
           (!maxDate.isValid() || date <= maxDate);
}

void SortFilterProxyModel::resetInternalData()
{
    emit filterReset();
}

bool SortFilterProxyModel::instrumentAccepted(const QString& instrument) const
{
    return acceptedInstruments.empty() || acceptedInstruments.contains(instrument) || (acceptedInstruments.contains("None") && instrument.isEmpty());
}

bool SortFilterProxyModel::objectAccepted(const QString& object) const
{
    return acceptedObjects.empty() || acceptedObjects.contains(object) || (acceptedObjects.contains("None") && object.isEmpty());
}

bool SortFilterProxyModel::filterAccepted(const QString& filter) const
{
    return acceptedFilters.empty() || acceptedFilters.contains(filter) || (acceptedFilters.contains("None") && filter.isEmpty());
}

bool SortFilterProxyModel::extensionAccepted(const QString& extension) const
{
    return acceptedExtensions.empty() || acceptedExtensions.contains(extension) || (acceptedExtensions.contains("None") && extension.isEmpty());
}

bool SortFilterProxyModel::folderAccepted(const QString& volume, const QString& folder) const
{
    QString folder2 = folder;
    if (!folder2.endsWith('/'))
        folder2.append('/');
    return acceptedVolume.isEmpty() || acceptedFolders.isEmpty() || (acceptedVolume == volume && acceptedFolders == folder2) || (acceptedVolume == volume && includeSubfolders && folder2.startsWith(acceptedFolders)) || (acceptedFolders.contains("None") && folder2.isEmpty());
}

bool SortFilterProxyModel::isDuplicateOf(QString hash) const
{
    return hash == this->duplicatesFilter;
}

void SortFilterProxyModel::setFilterMinimumDate(QDate date)
{
    minDate = date;
    invalidateFilter();
//    emit filterReset();
}

void SortFilterProxyModel::setFilterMaximumDate(QDate date)
{
    maxDate = date;
    invalidateFilter();
//    emit filterReset();
}

void SortFilterProxyModel::addAcceptedFilter(QString filterName)
{
    if (!acceptedFilters.contains(filterName))
    {
        acceptedFilters.append(filterName);
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedFilter(QString filterName)
{
    if (acceptedFilters.removeOne(filterName))
    {
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::addAcceptedInstrument(QString instrumentName)
{
    if (!acceptedInstruments.contains(instrumentName))
    {
        acceptedInstruments.append(instrumentName);
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedInstrument(QString instrumentName)
{
    if (acceptedInstruments.removeOne(instrumentName))
    {
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::addAcceptedObject(QString objectName)
{
    if (!acceptedObjects.contains(objectName))
    {
        acceptedObjects.append(objectName);
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedObject(QString objectName)
{
    if (acceptedObjects.removeOne(objectName))
    {
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::addAcceptedExtension(QString extensionName)
{
    if (!acceptedExtensions.contains(extensionName))
    {
        acceptedExtensions.append(extensionName);
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::removeAcceptedExtension(QString extensionName)
{
    if (acceptedExtensions.removeOne(extensionName))
    {
//        emit filterReset();
        invalidateFilter();
    }
}

void SortFilterProxyModel::addAcceptedFolder(QString volumeName, QString folderName, bool includeSubfolders)
{
    this->includeSubfolders = includeSubfolders;
    this->acceptedVolume = volumeName;
    acceptedFolders = folderName;
    invalidateFilter();

//    acceptedFolders.clear();
//    if (!acceptedFolders.contains(folderName))
//    {
//        acceptedFolders.append(folderName);
////        emit filterReset();
//        invalidateFilter();
//    }
}

void SortFilterProxyModel::removeAcceptedFolder(QString folderName)
{
//    if (acceptedFolders.removeOne(folderName))
//    {
////        emit filterReset();
//        invalidateFilter();
//    }
}

void SortFilterProxyModel::clearAcceptedFolders()
{
    acceptedFolders.clear();
    acceptedVolume.clear();
    invalidate();
}

void SortFilterProxyModel::activateDuplicatesFilter(bool shouldActivate)
{
    isDuplicatedFilterActive = shouldActivate;
    invalidate();
}

void SortFilterProxyModel::setDuplicatesFilter(QString filter)
{
    this->duplicatesFilter = filter;
}

void SortFilterProxyModel::setAstroFilesInFilter(const QSet<int> &astroFiles)
{
    acceptedAstroFilesId = astroFiles;
    filterIsActive = true;
    invalidate();
}

void SortFilterProxyModel::resetFilters()
{
    filterIsActive = false;
    qDebug()<<"Invalidating Filter";
    invalidate();
    qDebug()<<"Invalidated Filter";
}


