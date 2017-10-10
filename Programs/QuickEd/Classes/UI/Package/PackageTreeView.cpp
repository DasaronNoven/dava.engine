#include "Classes/UI/Package/PackageTreeView.h"
#include <QPainter>
#include <QHeaderView>
#include <QApplication>
#include <QStyleOptionButton>
#include <QMouseEvent>

PackageTreeView::PackageTreeView(QWidget* parent /*= NULL*/)
    : QTreeView(parent)
{
    viewport()->installEventFilter(this);
}

PackageTreeView::~PackageTreeView()
{
}

void PackageTreeView::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QTreeView::drawRow(painter, option, index);
    if (IsIndexCheckable(index))
    {
        QStyleOptionViewItem checkBoxOption(option);
        checkBoxOption.rect = checkBoxOption.rect;

        checkBoxOption.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        checkBoxOption.state = checkBoxOption.state & ~QStyle::State_HasFocus;

        switch (index.data(Qt::CheckStateRole).toInt())
        {
        case Qt::Unchecked:
            checkBoxOption.state |= QStyle::State_Off;
            break;
        case Qt::PartiallyChecked:
            checkBoxOption.state |= QStyle::State_NoChange;
            break;
        case Qt::Checked:
            checkBoxOption.state |= QStyle::State_On;
            break;
        }
        checkBoxOption.features |= QStyleOptionViewItem::HasCheckIndicator;

        checkBoxRect = style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &checkBoxOption, this);
        checkBoxOption.rect = checkBoxRect;

        style()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &checkBoxOption, painter, this);
    }
}

bool PackageTreeView::IsIndexCheckable(const QModelIndex& index) const
{
    return (index.flags() & Qt::ItemIsUserCheckable) && (index.data(Qt::CheckStateRole).isValid());
}

bool PackageTreeView::IsMouseUnderCheckBox(const QPoint& pos) const
{
    QModelIndex index = indexAt(pos);
    if (index.isValid())
    {
        if (IsIndexCheckable(index))
        {
            QRect rect = visualRect(index);
            rect.setX(checkBoxRect.x());
            rect.setSize(checkBoxRect.size());
            return rect.contains(pos);
        }
    }
    return false;
}

bool PackageTreeView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            QPoint pos = mouseEvent->pos();
            if (IsMouseUnderCheckBox(pos))
            {
                if (event->type() == QEvent::MouseButtonRelease)
                {
                    QModelIndex index = indexAt(pos);
                    QVariant checked = index.data(Qt::CheckStateRole);
                    Qt::CheckState newState = checked.toInt() == Qt::Checked ? Qt::Unchecked : Qt::Checked;
                    model()->setData(index, newState, Qt::CheckStateRole);
                    viewport()->update();
                }
                return true;
            }
        }
    }
    return QTreeView::eventFilter(obj, event);
}

void PackageTreeView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);
    connect(this->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)), this, SLOT(update()));
}
