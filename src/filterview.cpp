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

#include "astrofile.h"
#include "filterview.h"
#include "fileviewmodel.h"

#include <QCheckBox>
#include <QVBoxLayout>

FilterView::FilterView(QWidget *parent)
{
    _parent = parent;
    vLayout = new QVBoxLayout;
    parent->layout()->addWidget(createObjectsBox());
    parent->layout()->addWidget(createDateBox());
    parent->layout()->addWidget(createInstrumentsBox());
    parent->layout()->addWidget(createFiltersBox());
    parent->layout()->addWidget(createFileExtensionsBox());
    QSpacerItem * spacer = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    parent->layout()->addItem(spacer);
}

void FilterView::setFilterMinimumDate(QDate date)
{
    minDateEdit->setDate(date);

}

void FilterView::setFilterMaximumDate(QDate date)
{
    maxDateEdit->setDate(date);
}

void FilterView::searchFilterReset()
{
    resetGroups();
}

void FilterView::resetGroups()
{
    minDateEdit->setDate(QDate());
    maxDateEdit->setDate(QDate());
    addObjects();
    addDates();
    addInstruments();
    addFilters();
    addFileExtensions();
}

QWidget* FilterView::createObjectsBox()
{
    objectsGroup = new QGroupBox(tr("Objects"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(20);
    objectsGroup->setLayout(vbox);

    return objectsGroup;
}

QWidget* FilterView::createDateBox()
{
    datesGroup = new QGroupBox(tr("Dates"));
    minDateEdit = new QDateEdit();
    maxDateEdit = new QDateEdit();

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(minDateEdit);
    vbox->addWidget(maxDateEdit);
    vbox->addStretch(1);
    datesGroup->setLayout(vbox);
    minDateEdit->setDate(QDate::currentDate());

    connect(minDateEdit, &QDateEdit::dateChanged, this, &FilterView::minimumDateChanged);
    connect(maxDateEdit, &QDateEdit::dateChanged, this, &FilterView::maximumDateChanged);

    return datesGroup;
}

QWidget* FilterView::createInstrumentsBox()
{
    instrumentsGroup = new QGroupBox(tr("Instruments"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    instrumentsGroup->setLayout(vbox);

    return instrumentsGroup;
}

QWidget *FilterView::createFiltersBox()
{
    filtersGroup = new QGroupBox(tr("Filters"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    filtersGroup->setLayout(vbox);

    return filtersGroup;
}

QWidget *FilterView::createFileExtensionsBox()
{
    extensionsGroup = new QGroupBox(tr("Extensions"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    extensionsGroup->setLayout(vbox);

    return extensionsGroup;
}

void FilterView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i <= end; i++)
    {
        QModelIndex index = model()->index(i, 0, parent);
        auto data = model()->data(index);
        auto id = model()->data(index, AstroFileRoles::IdRole).toInt();
        auto object = model()->data(index, AstroFileRoles::ObjectRole).toString();
        auto instrument = model()->data(index, AstroFileRoles::InstrumentRole).toString();
        auto filter = model()->data(index, AstroFileRoles::FilterRole).toString();
        auto date = model()->data(index, AstroFileRoles::DateRole).toString();
        auto fullPath = model()->data(index, AstroFileRoles::FullPathRole).toString();
        auto fileExtension = model()->data(index, AstroFileRoles::FileExtensionRole).toString();

        if (acceptedAstroFiles.contains(id))
        {
            // The astrofile has already been added. Let's add it again
        }
        else
        {
            if (!object.isEmpty())
                fileTags["OBJECT"][object]++;
            if (!instrument.isEmpty())
                fileTags["INSTRUME"][instrument]++;
            if (!filter.isEmpty())
                fileTags["FILTER"][filter]++;
            if (!date.isEmpty())
                fileTags["DATE-OBS"][date]++;
            if (!fileExtension.isEmpty())
                fileTags["FILEEXT"][fileExtension]++;
            acceptedAstroFiles.insert(id);
        }
    }

    emit astroFileAdded(end-start+1);
    // We should not nuke all groups
    resetGroups();
}

void FilterView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i <= end; i++)
    {
        QModelIndex index = model()->index(i, 0, parent);
        auto data = model()->data(index);
        auto id = model()->data(index, AstroFileRoles::IdRole).toInt();
        auto object = model()->data(index, AstroFileRoles::ObjectRole).toString();
        auto instrument = model()->data(index, AstroFileRoles::InstrumentRole).toString();
        auto filter = model()->data(index, AstroFileRoles::FilterRole).toString();
        auto date = model()->data(index, AstroFileRoles::DateRole).toString();
        auto fullPath = model()->data(index, AstroFileRoles::FullPathRole).toString();
        auto fileExtension = model()->data(index, AstroFileRoles::FileExtensionRole).toString();

        if (acceptedAstroFiles.contains(id))
        {
            if (!object.isEmpty())
                fileTags["OBJECT"][object]--;
            if (!instrument.isEmpty())
                fileTags["INSTRUME"][instrument]--;
            if (!filter.isEmpty())
                fileTags["FILTER"][filter]--;
            if (!date.isEmpty())
                fileTags["DATE-OBS"][date]--;
            if (!fileExtension.isEmpty())
                fileTags["FILEEXT"][fileExtension]--;
            acceptedAstroFiles.remove(id);
        }
    }
    emit astroFileRemoved(end-start+1);
    // We should not nuke all groups
    resetGroups();
}

void FilterView::clearLayout(QLayout* layout)
{
    QLayoutItem* child;
    while ((child = layout->takeAt(0)) != 0)
    {
        delete child->widget();
        delete child;
    }
}

void FilterView::addDates()
{
    QDate minDate, maxDate;

    auto& o = fileTags["DATE-OBS"];
    QMapIterator setiter(o);
    while (setiter.hasNext())
    {
        QString n = setiter.next().key();
        QDate d = QDate::fromString(n, Qt::ISODate);
        if (d < minDate) minDate = d;
        if (d > maxDate) maxDate = d;
    }

    // Disable the date pickers until we fix them.
    minDateEdit->blockSignals(true);
    minDateEdit->setDate(minDate);
    minDateEdit->setReadOnly(true);
    minDateEdit->setEnabled(false);
    minDateEdit->blockSignals(false);

    maxDateEdit->blockSignals(true);
    maxDateEdit->setDate(maxDate);
    maxDateEdit->setReadOnly(true);
    maxDateEdit->setEnabled(false);
    maxDateEdit->blockSignals(false);
}

QCheckBox *FilterView::findCheckBox(QGroupBox *group, QList<QCheckBox *> &checkBoxes, QString titleProperty, void (FilterView::*func)(QString, int))
{
    for (auto& a: checkBoxes)
    {
        auto prop = a->property("for_name");
        if (prop == titleProperty)
        {
            return a;
        }
    }
    QCheckBox* checkBox = new QCheckBox();
    checkBox->setProperty("for_name", titleProperty);
    checkBox->setEnabled(true);
    checkBoxes.append(checkBox);
    group->layout()->addWidget(checkBox);
    connect(checkBox, &QCheckBox::stateChanged, this, [=]() {(this->*func)(titleProperty, checkBox->checkState());});
    return checkBox;
}

void FilterView::addObjects()
{
    auto& o = fileTags["OBJECT"];
    QMapIterator setiter(o);
    while (setiter.hasNext())
    {
        auto next = setiter.next();
        QString name = next.key();
        int num = next.value();
        QString tagText = QString("%1 (%2)").arg(name).arg(num);

        QCheckBox* checkBox = findCheckBox(objectsGroup, objectsCheckBoxes, name, &FilterView::selectedObjectsChanged);

        checkBox->setEnabled(num != 0);
        if (checkedTags.contains("OBJ_"+name))
            checkBox->setChecked(true);
        checkBox->setText(tagText);
    }
}

void FilterView::addInstruments()
{
    auto& o = fileTags["INSTRUME"];
    QMapIterator setiter(o);
    while (setiter.hasNext())
    {
        auto next = setiter.next();
        QString name = next.key();
        int num = next.value();
        QString tagText = QString("%1 (%2)").arg(name).arg(num);

        QCheckBox* checkBox = findCheckBox(instrumentsGroup, instrumentsCheckBoxes, name, &FilterView::selectedInstrumentsChanged);

        checkBox->setEnabled(num != 0);
        if (checkedTags.contains("INS_"+name))
            checkBox->setChecked(true);
        checkBox->setText(tagText);
    }
}

void FilterView::addFilters()
{
    auto& o = fileTags["FILTER"];
    QMapIterator setiter(o);
    while (setiter.hasNext())
    {
        auto next = setiter.next();
        QString name = next.key();
        int num = next.value();
        QString tagText = QString("%1 (%2)").arg(name).arg(num);

        QCheckBox* checkBox = findCheckBox(filtersGroup, filtersCheckBoxes, name, &FilterView::selectedFiltersChanged);

        checkBox->setEnabled(num != 0);
        if (checkedTags.contains("FIL_"+name))
            checkBox->setChecked(true);
        checkBox->setText(tagText);
    }
}

void FilterView::addFileExtensions()
{
    auto& o = fileTags["FILEEXT"];
    QMapIterator setiter(o);
    while (setiter.hasNext())
    {
        auto next = setiter.next();
        QString name = next.key();
        int num = next.value();
        QString tagText = QString("%1 (%2)").arg(name).arg(num);

        QCheckBox* checkBox = findCheckBox(extensionsGroup, extensionsCheckBoxes, name, &FilterView::selectedFileExtensionsChanged);

        checkBox->setEnabled(num != 0);
        if (checkedTags.contains("EXT_"+name))
            checkBox->setChecked(true);
        checkBox->setText(tagText);
    }
}

void FilterView::selectedObjectsChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        checkedTags.remove("OBJ_"+object);
        emit removeAcceptedObject(object);
        break;
    case 2:
        checkedTags.insert("OBJ_"+object);
        emit addAcceptedObject(object);
        break;
    }
}

void FilterView::selectedInstrumentsChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        checkedTags.remove("INS_"+object);
        emit removeAcceptedInstrument(object);
        break;
    case 2:
        checkedTags.insert("INS_"+object);
        emit addAcceptedInstrument(object);
        break;
    }
}

void FilterView::selectedFiltersChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        checkedTags.remove("FIL_"+object);
        emit removeAcceptedFilter(object);
        break;
    case 2:
        checkedTags.insert("FIL_"+object);
        emit addAcceptedFilter(object);
        break;
    }
}

void FilterView::selectedFileExtensionsChanged(QString object, int state)
{
    switch (state)
    {
    case 0:
        checkedTags.remove("EXT_"+object);
        emit removeAcceptedExtension(object);
        break;
    case 2:
        checkedTags.insert("EXT_"+object);
        emit addAcceptedExtension(object);
        break;
    }
}

QPaintEngine *FilterView::paintEngine() const
{
}

QRect FilterView::visualRect(const QModelIndex &index) const
{
}

void FilterView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
}

QModelIndex FilterView::indexAt(const QPoint &point) const
{
}

QModelIndex FilterView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
}

int FilterView::horizontalOffset() const
{
}

int FilterView::verticalOffset() const
{
}

bool FilterView::isIndexHidden(const QModelIndex &index) const
{
}

void FilterView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
}

QRegion FilterView::visualRegionForSelection(const QItemSelection &selection) const
{
}
