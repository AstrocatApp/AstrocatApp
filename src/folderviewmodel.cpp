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

#include "folderviewmodel.h"

#include <QDir>

FolderViewModel::FolderViewModel()
{
    rootItem = invisibleRootItem();
    rootFolder = new FolderNode();
}

QStringList foo(const QString& str) {
    QDir dir(str);
    QStringList folders;
    do
    {
        folders.prepend(dir.dirName());
        dir.cdUp();
    } while (!dir.isRoot());

    qDebug()<<"foo: " << folders;
    return folders;
}

void FolderViewModel::addItem(QString volume, QString folderPath)
{
    QStandardItem *parentItem = rootItem;
    FolderNode* iterator = rootFolder;
    int rootrow = 0;
    for (auto child : iterator->children)
    {
        if (child->folderName == volume)
        {
            iterator = child;
            parentItem = parentItem->child(rootrow);
            break;
        }
        rootrow++;
    }
    if (iterator == rootFolder)
    {
//        volumes.insert(volume);
        auto node = new FolderNode();
        node->folderName = volume;
        iterator = node;
        rootFolder->children.append(node);

        QStandardItem *item = new QStandardItem(volume);
        parentItem->appendRow(item);
        parentItem = item;
    }

    folders[folderPath]++;

    auto paths = foo(folderPath);
    for (auto path : paths)
    {
        qDebug()<<"Searching for: " << path;
        auto original = iterator;
        int row = 0;
        for (auto child : iterator->children)
        {
            if (child->folderName == path)
            {
                qDebug()<<"Iterating to: " << parentItem->child(row)->data(Qt::DisplayRole);
                parentItem = parentItem->child(row);
                iterator = child;
                break;
            }
            row++;
        }
        if (iterator == original)
        {
            // we did not find it
            qDebug()<< "Did not find: " << path;
            auto node = new FolderNode();
            node->folderName = path;
            iterator->children.append(node);
            iterator = node;

            QStandardItem *item = new QStandardItem(path);
            qDebug()<<"Adding to parent: " << parentItem->data(Qt::DisplayRole).toString() << " item: " <<path;
            parentItem->appendRow(item);
            parentItem = item;
        }
        else
        {
            // we found it
            continue;
        }
    }
}

void FolderViewModel::removeItem(QString volume, QString folderPath)
{
    folders[folderPath]--;
}

