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

#include "filtergroupbox.h"

#include <QToolButton>
#include <QStyle>
#include <QVBoxLayout>

FilterGroupBox::FilterGroupBox(QWidget *parent) : QGroupBox(parent)
{
    optionsButton = new QToolButton(this);
    optionsButton->setFixedWidth(16);
    optionsButton->setFixedHeight(16);
    optionsButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    optionsButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    optionsButton->setArrowType(Qt::ArrowType::NoArrow);

    optionsButton->move(120, 0);

    collapseButton = new QToolButton(this);
    collapseButton->setFixedWidth(16);
    collapseButton->setFixedHeight(16);
    collapseButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

    collapseButton->setArrowType(Qt::ArrowType::DownArrow);

    connect(collapseButton, SIGNAL(clicked()), this, SLOT(collapse()));

    this->setStyleSheet(
                "QGroupBox::title {"
                    "top: 0px;"
                    "left: 24px;"
                    "color: white;"
                "}"
                "QGroupBox::indicator {"
                    "top: 50px;"
                    "left: 0px;"
                "}"
                );

    int animationDuration = 100;
    collapseAnimation = new QPropertyAnimation(this, "maximumHeight");
    collapseAnimation->setDuration(animationDuration);
    collapseAnimation->setEndValue(19);

    expandAnimation = new QPropertyAnimation(this, "maximumHeight");
    expandAnimation->setDuration(animationDuration);
    expandAnimation->setStartValue(20);
}

FilterGroupBox::~FilterGroupBox()
{
}

void FilterGroupBox::addToolButtonMenu(QMenu* menu)
{
    optionsButton->setMenu(menu);
}

void FilterGroupBox::collapse()
{
    if (expandedSize == 0)
    {
        expandedSize = this->sizeHint().height();
        collapseAnimation->setStartValue(expandedSize);
        expandAnimation->setEndValue(expandedSize);
    }
    if (isExpanded)
    {
        // Collapsing now
        collapseButton->setArrowType(Qt::ArrowType::RightArrow);
        collapseAnimation->start();
    }
    else
    {
        // Expanding now
        collapseButton->setArrowType(Qt::ArrowType::DownArrow);
        expandAnimation->start();
    }

    isExpanded = !isExpanded;
}
