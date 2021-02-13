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

#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include "astrofile.h"

#include <QDateEdit>
#include <QGroupBox>
#include <QWidget>

class FilterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FilterWidget(QWidget *parent = nullptr);

signals:
    void minimumDateChanged(QDate date);
    void maximumDateChanged(QDate date);

public slots:
    void setFilterMinimumDate(QDate date);
    void setFilterMaximumDate(QDate date);
    void addAstroFileTags(const AstroFile& astroFile);
    void searchFilterReset();
    void setAllTags(const QMap<QString, QSet<QString>>& tags);

private:
    QGroupBox* objectsGroup;
    QGroupBox* instrumentsGroup;
    QGroupBox* filtersGroup;
    QGroupBox* datesGroup;

    QDateEdit* minDateEdit;
    QDateEdit* maxDateEdit;

    QWidget* CreateDateBox();
    QWidget* CreateObjectsBox();
    QWidget* CreateInstrumentsBox();
    QWidget* CreateFiltersBox();
    void addObjects();
    void addDates();
    void addInstruments();
    void addFilters();
    void ClearLayout(QLayout* layout);
    void ResetGroups();
    QMap<QString, QSet<QString>> fileTags;
};

#endif // FILTERWIDGET_H
