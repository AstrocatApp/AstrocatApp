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
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

FilterView::FilterView(QWidget *parent)
{
    _parent = parent;
    vLayout = new QVBoxLayout();

    folderModel = new FolderViewModel();

    createGroupBoxes();
    addGroupBoxesToLayout();
    QSpacerItem * spacer = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    _parent->layout()->addItem(spacer);

    setFoldersModel(folderModel);

    folderTreeSelectionModel = new QItemSelectionModel(folderModel);
    foldersTreeView->setSelectionModel(folderTreeSelectionModel);
    foldersTreeView->setSelectionMode(SelectionMode::SingleSelection);
    foldersTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(folderTreeSelectionModel, &QItemSelectionModel::selectionChanged, this, &FilterView::treeViewClicked);
//    connect(folderModel, &QStandardItemModel::itemChanged, this, &FilterView::itemChanged);
}

void FilterView::createGroupBoxes()
{
    createObjectsBox();
    createDateBox();
    createInstrumentsBox();
    createFiltersBox();
    createFileExtensionsBox();
    createFoldersBox();
}

void FilterView::addGroupBoxesToLayout()
{
    _parent->layout()->addWidget(objectsGroup);
//    _parent->layout()->addWidget(datesGroup);
    _parent->layout()->addWidget(instrumentsGroup);
    _parent->layout()->addWidget(filtersGroup);
    _parent->layout()->addWidget(extensionsGroup);
    _parent->layout()->addWidget(foldersGroup);
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

void FilterView::foldersIncludeSubfolders()
{
    bFoldersIncludeSubfolders = !bFoldersIncludeSubfolders;

    // We will simulate a click event on the tree to enforce the new setting
    auto selection = foldersTreeView->selectionModel()->selection();
    if (!selection.isEmpty())
    {
        emit treeViewClicked(selection, QItemSelection());
    }
}

void FilterView::resetFoldersSelection()
{
    this->foldersTreeView->clearSelection();
//    emit treeViewClicked(QItemSelection(), QItemSelection());
//    selectedFoldersChanged(QString(), QString(), 0);
}

void FilterView::setFoldersModel(QAbstractItemModel* model)
{
    this->foldersTreeView->setModel(model);
}

void FilterView::treeViewClicked(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (selected.isEmpty())
    {
        selectedFoldersChanged(QString(), QString(), 3);
        return;
    }
    QModelIndex index = selected[0].indexes()[0];
    QStringList paths;
    QString volumeRoot = "";
    auto itIndex = index;
    while (itIndex != QModelIndex())
    {
        auto folder = folderModel->data(itIndex, Qt::DisplayRole).toString();
        auto data = folderModel->data(itIndex, Qt::UserRole + 2); // TODO: set a well-defined user role for this
        if (data.isValid())
        {
            auto node = data.value<FolderNode*>();
            volumeRoot = node->getRoot();
        }
        paths.prepend(folder);
        itIndex = itIndex.parent();
    }

    int counter = 0;
    QString volume;
    QString fullPath;
    for (auto& path : paths)
    {
        if (counter == 0)
        {
            volume = path;
            counter++;
            continue;
        }
        fullPath = fullPath + path;
        if (!fullPath.endsWith('/'))
        {
            fullPath.append('/');
        }
    }
    if (!volumeRoot.endsWith("/"))
    {
        volumeRoot.append("/");
    }
    fullPath.prepend(volumeRoot);
    if (!fullPath.endsWith("/"))
    {
        fullPath.append("/");
    }

    selectedFoldersChanged(volume, fullPath, 2);
}

void FilterView::resetGroups()
{
    minDateEdit->setDate(QDate());
    maxDateEdit->setDate(QDate());
    addObjects();
//    addDates();
    addInstruments();
    addFilters();
    addFileExtensions();
}

FilterGroupBox* FilterView::createGenericBox(QString title)
{
    auto groupBox = new FilterGroupBox(title);

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addStretch(1);
    groupBox->setLayout(vbox);

    return groupBox;
}

QWidget* FilterView::createObjectsBox()
{
    objectsGroup = createGenericBox(tr("Objects"));
    return objectsGroup;
}

QWidget* FilterView::createDateBox()
{
    datesGroup = createGenericBox(tr("Dates"));
    minDateEdit = new QDateEdit();
    maxDateEdit = new QDateEdit();

    minDateEdit->setToolTip("Feature Coming Soon");
    maxDateEdit->setToolTip("Feature Coming Soon");

    auto vbox = datesGroup->layout();
    vbox->addWidget(minDateEdit);
    vbox->addWidget(maxDateEdit);
    minDateEdit->setDate(QDate::currentDate());

    connect(minDateEdit, &QDateEdit::dateChanged, this, &FilterView::minimumDateChanged);
    connect(maxDateEdit, &QDateEdit::dateChanged, this, &FilterView::maximumDateChanged);

    return datesGroup;
}

QWidget* FilterView::createInstrumentsBox()
{
    instrumentsGroup = createGenericBox(tr("Instruments"));
    return instrumentsGroup;
}

QWidget *FilterView::createFiltersBox()
{
    filtersGroup = createGenericBox(tr("Filters"));
    return filtersGroup;
}

QWidget *FilterView::createFileExtensionsBox()
{
    extensionsGroup = createGenericBox(tr("Extensions"));
    return extensionsGroup;
}

QWidget *FilterView::createFoldersBox()
{
    foldersGroup = createGenericBox(tr("Folders"));

    foldersTreeView = new QTreeView();
    foldersTreeView->setHeaderHidden(true);
    auto vbox = foldersGroup->layout();
    vbox->addWidget(foldersTreeView);

    QMenu* myMenu = createFoldersOptionsMenu();
    foldersGroup->addToolButtonMenu(myMenu);

    return foldersGroup;
}

QMenu *FilterView::createFoldersOptionsMenu()
{
    QMenu* myMenu = new QMenu();

    QAction* includeSubfoldersAction = new QAction(tr("Include Subfolders"), this);
    includeSubfoldersAction->setCheckable(true);
    includeSubfoldersAction->setChecked(true);
    myMenu->addAction(includeSubfoldersAction);

    QObject::connect(includeSubfoldersAction, &QAction::triggered, this, &FilterView::foldersIncludeSubfolders);

    QAction* resetSelectionAction = new QAction(tr("Clear Selection"), this);
    myMenu->addAction(resetSelectionAction);

    QObject::connect(resetSelectionAction, &QAction::triggered, this, &FilterView::resetFoldersSelection);

    return myMenu;
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
        auto directoryPath = model()->data(index, AstroFileRoles::DirectoryRole).toString();
        auto volumeName = model()->data(index, AstroFileRoles::VolumeNameRole).toString();
        auto volumeRoot = model()->data(index, AstroFileRoles::VolumeRootRole).toString();
        auto fileExtension = model()->data(index, AstroFileRoles::FileExtensionRole).toString();
        auto imageType = model()->data(index, AstroFileRoles::CalframeRole).toString();

        // We should check if there is a performance improvement of getting the object
        // instead of asking for data from the model.
//        auto astrofile = model()->data(index, AstroFileRoles::ItemRole).value<AstroFile*>();

//        auto id = astrofile->Id;
//        auto object = astrofile->Tags["OBJECT"];
//        auto instrument = astrofile->Tags["INSTRUME"];
//        auto filter = astrofile->Tags["FILTER"];
//        auto date = astrofile->Tags["DATE-OBS"];
//        auto fullPath = astrofile->FullPath;
//        auto directoryPath = astrofile->DirectoryPath;
//        auto volumeName = astrofile->VolumeName;
//        auto volumeRoot = astrofile->VolumeRoot;
//        auto fileExtension = astrofile->FileExtension;
//        auto imageType = astrofile->Tags["CALFRAME"];

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
            acceptedFolders[directoryPath]++;
            acceptedAstroFiles.insert(id);
            folderModel->addItem(volumeName, volumeRoot, directoryPath);
        }
    }

//    foldersTreeView->expandToDepth(2);
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
        auto directoryPath = model()->data(index, AstroFileRoles::DirectoryRole).toString();
        auto volumeName = model()->data(index, AstroFileRoles::VolumeNameRole).toString();
        auto fileExtension = model()->data(index, AstroFileRoles::FileExtensionRole).toString();
        auto imageType = model()->data(index, AstroFileRoles::CalframeRole).toString();

        // We should check if there is a performance improvement of getting the object
        // instead of asking for data from the model.
//        auto astrofile = model()->data(index, AstroFileRoles::ItemRole).value<AstroFile*>();

//        auto id = astrofile->Id;
//        auto object = astrofile->Tags["OBJECT"];
//        auto instrument = astrofile->Tags["INSTRUME"];
//        auto filter = astrofile->Tags["FILTER"];
//        auto date = astrofile->Tags["DATE-OBS"];
//        auto fullPath = astrofile->FullPath;
//        auto directoryPath = astrofile->DirectoryPath;
//        auto volumeName = astrofile->VolumeName;
//        auto volumeRoot = astrofile->VolumeRoot;
//        auto fileExtension = astrofile->FileExtension;
//        auto imageType = astrofile->Tags["CALFRAME"];

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
            acceptedFolders[directoryPath]--;
            acceptedAstroFiles.remove(id);
            folderModel->removeItem(volumeName, directoryPath);
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

#ifdef Q_OS_WINDOWS
    checkBox->setStyleSheet(
                    "QCheckBox::indicator:disabled{background-color:gray;}"
                    "QWidget {color: white;}"
                    );
#endif
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

void FilterView::selectedFoldersChanged(QString volume, QString folder, int state)
{
    switch (state)
    {
    case 0:
//        checkedTags.remove("FOL_"+folder);
        emit removeAcceptedFolder(folder);
        break;
    case 2:
//        checkedTags.insert("FOL_"+folder);
        emit addAcceptedFolder(volume, folder, this->bFoldersIncludeSubfolders);
        break;
    case 3:
        emit resetAcceptedFolders();
    }
}

//void FilterView::itemChanged(QStandardItem *item)
//{
//    qDebug()<<"item changed:" << item->data(Qt::UserRole + 1);
//}
