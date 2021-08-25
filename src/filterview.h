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
#include "filtergroupbox.h"
#include "folderviewmodel.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDateEdit>
#include <QGroupBox>
#include <QListView>
#include <QObject>
#include <QTreeView>
#include <QVBoxLayout>

class FilterView : public QListView
{
    Q_OBJECT
public:
    explicit FilterView(QWidget *parent = nullptr);

public slots:
    void setFilterMinimumDate(QDate date);
    void setFilterMaximumDate(QDate date);
    void searchFilterReset();

    void foldersIncludeSubfolders();
    void resetFoldersSelection();

    void setFoldersModel(QAbstractItemModel* model);
    void treeViewClicked(const QItemSelection &selected, const QItemSelection &deselected);

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
    void addAcceptedFolder(QString volumeName, QString folderName, bool includeSubfolders);
    void removeAcceptedFolder(QString objectName);
    void resetAcceptedFolders();
    void astroFileAdded(int numberAdded);
    void astroFileRemoved(int numberRemoved);

private:
    QWidget* _parent;
    QVBoxLayout* vLayout;
    FilterGroupBox* objectsGroup;
    FilterGroupBox* instrumentsGroup;
    FilterGroupBox* filtersGroup;
    FilterGroupBox* extensionsGroup;
    FilterGroupBox* datesGroup;
    FilterGroupBox* foldersGroup;
    QDateEdit* minDateEdit;
    QDateEdit* maxDateEdit;
    QTreeView* foldersTreeView;
    QItemSelectionModel* folderTreeSelectionModel;

    FilterGroupBox* myGroup;
    FolderViewModel* folderModel;

    QList<QCheckBox*> objectsCheckBoxes;
    QList<QCheckBox*> instrumentsCheckBoxes;
    QList<QCheckBox*> filtersCheckBoxes;
    QList<QCheckBox*> extensionsCheckBoxes;
    QList<QCheckBox*> foldersCheckBoxes;
    QCheckBox* findCheckBox(QGroupBox* group, QList<QCheckBox*>& checkBoxes, QString titleProperty, void (FilterView::* func)(QString,int));

    FilterGroupBox* createGenericBox(QString title);
    QWidget* createDateBox();
    QWidget* createObjectsBox();
    QWidget* createInstrumentsBox();
    QWidget* createFiltersBox();
    QWidget* createFileExtensionsBox();
    QWidget* createFoldersBox();

    QMenu* createFoldersOptionsMenu();

    QSet<int> acceptedAstroFiles;
    QMap<QString, QMap<QString,int>> fileTags;
    QMap<QString, int> acceptedFolders;
    QSet<QString> checkedTags;

    bool bFoldersIncludeSubfolders = true;

    void createGroupBoxes();
    void addGroupBoxesToLayout();
    void addObjects();
    void addDates();
    void addInstruments();
    void addFilters();
    void addFileExtensions();
//    void addFolders();
    void resetGroups();
    void clearLayout(QLayout* layout);
    void selectedObjectsChanged(QString object, int state);
    void selectedInstrumentsChanged(QString object, int state);
    void selectedFiltersChanged(QString object, int state);
    void selectedFileExtensionsChanged(QString object, int state);
    void selectedFoldersChanged(QString volume, QString folder, int state);

//    void itemChanged(QStandardItem *item);

    // QAbstractItemView interface
protected slots:
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;

};

#endif // FILTERVIEW_H
