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

#ifndef FILTERVIEW_H
#define FILTERVIEW_H

#include "astrofile.h"

#include <QAbstractItemView>
#include <QDateEdit>
#include <QGroupBox>
#include <QObject>
#include <QVBoxLayout>

class FilterView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit FilterView(QWidget *parent = nullptr);

public slots:
    void setFilterMinimumDate(QDate date);
    void setFilterMaximumDate(QDate date);
    void addAstroFileTags(const AstroFile& astroFile);
    void searchFilterReset();

signals:
    void minimumDateChanged(QDate date);
    void maximumDateChanged(QDate date);
    void addAcceptedFilter(QString filterName);
    void removeAcceptedFilter(QString filterName);
    void addAcceptedInstrument(QString instrumentName);
    void removeAcceptedInstrument(QString instrumentName);
    void addAcceptedObject(QString objectName);
    void removeAcceptedObject(QString objectName);
    void addAcceptedExtension(QString objectName);
    void removeAcceptedExtension(QString objectName);
    void astroFileAdded(int numberAdded);
    void astroFileRemoved(int numberRemoved);

private:
    QWidget* _parent;
    QVBoxLayout* vLayout;
    QGroupBox* objectsGroup;
    QGroupBox* instrumentsGroup;
    QGroupBox* filtersGroup;
    QGroupBox* extensionsGroup;
    QGroupBox* datesGroup;
    QDateEdit* minDateEdit;
    QDateEdit* maxDateEdit;
    QWidget* createDateBox();
    QWidget* createObjectsBox();
    QWidget* createInstrumentsBox();
    QWidget* createFiltersBox();
    QWidget* createFileExtensionsBox();
    QSet<QString> acceptedAstroFiles;
    QMap<QString, QMap<QString,int>> fileTags;
    QSet<QString> checkedTags;
    void addObjects();
    void addDates();
    void addInstruments();
    void addFilters();
    void addFileExtensions();
    void resetGroups();
    void clearLayout(QLayout* layout);
    void selectedObjectsChanged(QString object, int state);
    void selectedInstrumentsChanged(QString object, int state);
    void selectedFiltersChanged(QString object, int state);
    void selectedFileExtensionsChanged(QString object, int state);

    // QPaintDevice interface
public:
    QPaintEngine *paintEngine() const;

    // QAbstractItemView interface
public:
    QRect visualRect(const QModelIndex &index) const;
    void scrollTo(const QModelIndex &index, ScrollHint hint);
    QModelIndex indexAt(const QPoint &point) const;

protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    int horizontalOffset() const;
    int verticalOffset() const;
    bool isIndexHidden(const QModelIndex &index) const;
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command);
    QRegion visualRegionForSelection(const QItemSelection &selection) const;

    // QAbstractItemView interface
protected slots:
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
};

#endif // FILTERVIEW_H
