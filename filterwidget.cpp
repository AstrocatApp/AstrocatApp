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
    vLayout->addWidget(createObjectsBox());
    vLayout->addWidget(createDateBox());
    vLayout->addWidget(createInstrumentsBox());
    vLayout->addWidget(createFiltersBox());
    setLayout(vLayout);
    parent->layout()->addWidget(this);
}

QWidget* FilterWidget::createObjectsBox()
{
    objectsGroup = new QGroupBox(tr("Objects"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    objectsGroup->setLayout(vbox);

    return objectsGroup;
}

QWidget* FilterWidget::createDateBox()
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

QWidget* FilterWidget::createInstrumentsBox()
{
    instrumentsGroup = new QGroupBox(tr("Instruments"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    instrumentsGroup->setLayout(vbox);

    return instrumentsGroup;
}

QWidget *FilterWidget::createFiltersBox()
{
    filtersGroup = new QGroupBox(tr("Filters"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    filtersGroup->setLayout(vbox);

    return filtersGroup;
}

void FilterWidget::clearLayout(QLayout* layout)
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
    minDateEdit->setDate(date);
}

void FilterWidget::setFilterMaximumDate(QDate date)
{
    maxDateEdit->setDate(date);
}

void FilterWidget::addAstroFileTags(const AstroFile &astroFile)
{
    bool newTagFound = false;
    QString t = astroFile.Tags["OBJECT"];
    auto& x= t.remove("'");
    if (x.isEmpty())
        x = "None";
    if (!fileTags["OBJECT"].contains(x))
    {
        fileTags["OBJECT"].insert(x);
        newTagFound = true;
    }
    t = astroFile.Tags["INSTRUME"];
    x= t.remove("'");
    if (x.isEmpty())
        x = "None";
    if (!fileTags["INSTRUME"].contains(x))
    {
        fileTags["INSTRUME"].insert(x);
        newTagFound = true;
    }
    t = astroFile.Tags["FILTER"];
    x= t.remove("'");
    if (x.isEmpty())
        x = "None";
    if (!fileTags["FILTER"].contains(x))
    {
        fileTags["FILTER"].insert(x);
        newTagFound = true;
    }
    t = astroFile.Tags["DATE-OBS"];
    x= t.remove("'");
    if (x.isEmpty())
        x = "None";
    if (!fileTags["DATE-OBS"].contains(x))
    {
        fileTags["DATE-OBS"].insert(x);
        newTagFound = true;
    }
    if (newTagFound)
        resetGroups();
}

void FilterWidget::searchFilterReset()
{
    fileTags.clear();
    resetGroups();
}

void FilterWidget::resetGroups()
{
    addObjects();
    addDates();
    addInstruments();
    addFilters();
}

void FilterWidget::setAllTags(const QMap<QString, QSet<QString> > &tags)
{
    fileTags.clear();
    fileTags.insert(tags);
    resetGroups();
}

void FilterWidget::addDates()
{
    auto& o = fileTags["DATE-OBS"];
    QSetIterator setiter(o);
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QDate d = QDate::fromString(n, Qt::ISODate);
        if (d < minDateEdit->date())
        {
            minDateEdit->setDate(d);
        }
        if (d > maxDateEdit->date())
        {
            maxDateEdit->setDate(d);
        }
    }
}

void FilterWidget::addObjects()
{
    clearLayout(objectsGroup->layout());
    auto& o = fileTags["OBJECT"];
    QSetIterator setiter(o);
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QCheckBox* chekBox = new QCheckBox(n);
        objectsGroup->layout()->addWidget(chekBox);
        connect(chekBox, &QCheckBox::stateChanged, this, [=]() {selectedObjectsChanged(chekBox->text(), chekBox->checkState());});
    }
}

void FilterWidget::addInstruments()
{
    clearLayout(instrumentsGroup->layout());
    auto& o = fileTags["INSTRUME"];
    QSetIterator setiter(o);
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QCheckBox* chekBox = new QCheckBox(n);
        instrumentsGroup->layout()->addWidget(chekBox);
        connect(chekBox, &QCheckBox::stateChanged, this, [=]() {selectedInstrumentsChanged(chekBox->text(), chekBox->checkState());});
    }
}

void FilterWidget::addFilters()
{
    clearLayout(filtersGroup->layout());
    auto& o = fileTags["FILTER"];
    QSetIterator setiter(o);
    while (setiter.hasNext())
    {
        QString n = setiter.next();
        n.remove("'");
        QCheckBox* chekBox = new QCheckBox(n);
        filtersGroup->layout()->addWidget(chekBox);
        connect(chekBox, &QCheckBox::stateChanged, this, [=]() {selectedFiltersChanged(chekBox->text(), chekBox->checkState());});
    }
}

void FilterWidget::selectedObjectsChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        emit removeAcceptedObject(object);
        break;
    case 2:
        emit addAcceptedObject(object);
        break;
    }
}

void FilterWidget::selectedInstrumentsChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        emit removeAcceptedInstrument(object);
        break;
    case 2:
        emit addAcceptedInstrument(object);
        break;
    }
}

void FilterWidget::selectedFiltersChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        emit removeAcceptedFilter(object);
        break;
    case 2:
        emit addAcceptedFilter(object);
        break;
    }
}
