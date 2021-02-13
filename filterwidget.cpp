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

#include "filterwidget.h"

#include <QCheckBox>
#include <QDate>
#include <QRadioButton>
#include <QVBoxLayout>

FilterWidget::FilterWidget(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->addWidget(CreateObjectsBox());
    vLayout->addWidget(CreateDateBox());
    vLayout->addWidget(CreateInstrumentsBox());
    vLayout->addWidget(CreateFiltersBox());
    setLayout(vLayout);
    parent->layout()->addWidget(this);
}

QWidget* FilterWidget::CreateObjectsBox()
{
    objectsGroup = new QGroupBox(tr("Objects"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    objectsGroup->setLayout(vbox);

    return objectsGroup;
}

QWidget* FilterWidget::CreateDateBox()
{
    datesGroup = new QGroupBox(tr("Dates"));
    minDateEdit = new QDateEdit();
    maxDateEdit = new QDateEdit();

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(minDateEdit);
    vbox->addWidget(maxDateEdit);
    vbox->addStretch(1);
    datesGroup->setLayout(vbox);

    connect(minDateEdit, &QDateEdit::dateChanged, this, &FilterWidget::minimumDateChanged);
    connect(maxDateEdit, &QDateEdit::dateChanged, this, &FilterWidget::maximumDateChanged);

    return datesGroup;
}

QWidget* FilterWidget::CreateInstrumentsBox()
{
    instrumentsGroup = new QGroupBox(tr("Instruments"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    instrumentsGroup->setLayout(vbox);

    return instrumentsGroup;
}

QWidget *FilterWidget::CreateFiltersBox()
{
    filtersGroup = new QGroupBox(tr("Filters"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    filtersGroup->setLayout(vbox);

    return filtersGroup;

}

void FilterWidget::ClearLayout(QLayout* layout)
{
    QLayoutItem* child;
    while ((child = layout->takeAt(0)) != 0)
    {
        delete child->widget();
        delete child;
    }
}

void FilterWidget::setFilterMinimumDate(QDate date)
{
    qDebug() << "FilterWidget::setFilterMinimumDate";
    minDateEdit->setDate(date);
}

void FilterWidget::setFilterMaximumDate(QDate date)
{
    qDebug() << "FilterWidget::setFilterMaximumDate";
    maxDateEdit->setDate(date);
}

void FilterWidget::addAstroFileTags(const AstroFile &astroFile)
{
    bool newTagFound = false;
    QString t = astroFile.Tags["OBJECT"];
    auto& x= t.remove("'");
    if (!fileTags["OBJECT"].contains(x))
    {
        fileTags["OBJECT"].insert(x);
        newTagFound = true;
    }
    t = astroFile.Tags["INSTRUME"];
    x= t.remove("'");
    if (!fileTags["INSTRUME"].contains(x))
    {
        fileTags["INSTRUME"].insert(x);
        newTagFound = true;
    }
    t = astroFile.Tags["FILTER"];
    x= t.remove("'");
    if (!fileTags["FILTER"].contains(x))
    {
        fileTags["FILTER"].insert(x);
        newTagFound = true;
    }
//    t = astroFile.Tags["OBJECT"];
//    x= t.remove("'");
//    if (!fileTags["DATEXXX"].contains(x))
//    {
//        fileTags["DATEXXX"].insert(x);
//        newTagFound = true;
//    }
    if (newTagFound)
        ResetGroups();
}

void FilterWidget::searchFilterReset()
{
    fileTags.clear();
    ResetGroups();
    qDebug() << "Search Filter has been reset";
}

void FilterWidget::ResetGroups()
{
    qDebug() << "Resetting Groups";
    addObjects();
    addDates();
    addInstruments();
    addFilters();
}

void FilterWidget::setAllTags(const QMap<QString, QSet<QString> > &tags)
{
    fileTags.clear();
    fileTags.insert(tags);
    ResetGroups();
}

void FilterWidget::addObjects()
{
    ClearLayout(objectsGroup->layout());
    auto& o = fileTags["OBJECT"];
    QSetIterator setiter(o);
//    objectsGroup->layout()->invalidate();
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QCheckBox* chekBox = new QCheckBox(n);
        objectsGroup->layout()->addWidget(chekBox);
    }
//    objectsGroup->layout()->update();
}

void FilterWidget::addDates()
{
//    auto& o = fileTags["OBJECT"];
//    QSetIterator setiter(o);
//    while (setiter.hasNext())
//    {
//        qDebug() << "+++++" << setiter.next();
//    }
}

void FilterWidget::addInstruments()
{
    ClearLayout(instrumentsGroup->layout());
    auto& o = fileTags["INSTRUME"];
    QSetIterator setiter(o);
//    instrumentsGroup->layout()->invalidate();
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QCheckBox* chekBox = new QCheckBox(n);
        instrumentsGroup->layout()->addWidget(chekBox);
    }
//    instrumentsGroup->layout()->update();
}

void FilterWidget::addFilters()
{
    ClearLayout(filtersGroup->layout());
    auto& o = fileTags["FILTER"];
    QSetIterator setiter(o);
//    filtersGroup->layout()->invalidate();
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QCheckBox* chekBox = new QCheckBox(n);
        filtersGroup->layout()->addWidget(chekBox);
    }
//    filtersGroup->layout()->update();
}
