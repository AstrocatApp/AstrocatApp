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
    void addAcceptedExtension(QString extensionName);
    void removeAcceptedExtension(QString extensionName);
    void addAcceptedFolder(QString folderName, bool includeSubfolders);
    void removeAcceptedFolder(QString folderName);
    void activateDuplicatesFilter(bool shouldActivate);
    void setDuplicatesFilter(QString filter);

signals:
    void filterMinimumDateChanged(QDate date);
    void filterMaximumDateChanged(QDate date);
    void filterReset();

protected:
    // QSortFilterProxyModel interface
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
    QDate minDate;
    QDate maxDate;
    QList<QString> acceptedFilters;
    QList<QString> acceptedObjects;
    QList<QString> acceptedInstruments;
    QList<QString> acceptedExtensions;
//    QList<QString> acceptedFolders;
    QString acceptedFolders;
    bool dateInRange(QDate date) const;
    bool instrumentAccepted(QString instrument) const;
    bool objectAccepted(QString object) const;
    bool filterAccepted(QString filter) const;
    bool extensionAccepted(QString filter) const;
    bool folderAccepted(QString folder) const;
    bool isDuplicatedFilterActive;
    QString duplicatesFilter;
    bool isDuplicateOf(QString hash) const;
    bool includeSubfolders = true;

protected slots:
    virtual void resetInternalData();
};

#endif // SORTFILTERPROXYMODEL_H
