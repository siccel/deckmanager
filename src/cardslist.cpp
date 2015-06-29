#include "cardslist.h"
#include "limitcards.h"
#include <QDebug>

CardsList::CardsList(QWidget *parent, bool &_moved)
    : QWidget(parent), pos(0), cardSize(177 / 3.5, 254 / 3.5), cardsPerColumn(0),
      moved(_moved), sb(nullptr), needRefreshId(false), current(-1)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setMinimumWidth(cardSize.width() + fontMetrics().width(tr("宽")) * 10);
    auto family = font().family();
    setFont(QFont(family, 11));
}

void CardsList::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    needRefreshId = true;
    point = event->pos();

    setPos(pos - numSteps);
    event->accept();
}

void CardsList::refresh()
{
    int max = ls.size() - cardsPerColumn;
    if(sb)
    {
        sb->setMaximum(max > 0 ? max : 0);
    }
}

void CardsList::setScrollBar(QScrollBar *_sb)
{
    sb = _sb;
    sb->setMaximum(ls.size());
    connect(sb, SIGNAL(valueChanged(int)), this, SLOT(setPos(int)));
}

QString CardsList::adToString(int ad)
{
    if(ad == -2)
    {
        return "?";
    }
    else
    {
        return std::move(QString::number(ad));
    }
}

void CardsList::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if(!ls.empty())
    {
        int fmHeight = painter.fontMetrics().height();
        int h = height(), cardHeight = cardSize.height(),
                cardWidth = cardSize.width();

        cardsPerColumn = h / cardHeight;
        int lsSize = static_cast<std::size_t>(ls.size());

        QHash<int, CardItem> newItems;

        double varHeight = h - cardHeight * 1.0;

        for(int i = 0; i < cardsPerColumn && i + pos < lsSize; i++)
        {
            int id = ls[i + pos];

            auto it = newItems.insert(i + pos, CardItem(id));

            auto &item = it.value();

            int y = varHeight * i / (cardsPerColumn - 1);


            current = itemAt(mapFromGlobal(QCursor::pos()));
            if(i + pos == current)
            {
                QBrush brush = painter.brush();
                QPen pen = painter.pen();
                painter.setPen(Qt::transparent);
                painter.setBrush(Qt::lightGray);
                painter.drawRect(QRect(QPoint(0,  y), QSize(width(), cardSize.height())));
                painter.setBrush(brush);
                painter.setPen(pen);
            }

            item.setPos(QPoint(0, y));

            painter.drawPixmap(0, y, cardWidth, cardHeight,
                               *item.getPixmap().data());

            int lim = LimitCards::getLimit(it->getId());
            if(lim < 3)
            {
                auto data = LimitCards::getPixmap(lim);
                if(data)
                {
                    painter.drawPixmap(0, y, 16, 16, *data.data());
                }
            }

            auto card = CardPool::getCard(id);

            if(!card)
            {
                continue;
            }

            painter.drawText(cardWidth + 5, y + fmHeight, card->name);
            QString ot;
            if((card->ot & 0x3) == 1)
            {
                ot = tr("[OCG]");
            }
            else if((card->ot & 0x3) == 2)
            {
                ot = tr("[TCG]");
            }

            if(card->type & Card::TYPE_MONSTER)
            {
                painter.drawText(cardWidth + 5, y + 5 + fmHeight * 2,
                                 card->cardRace() + tr("/") + card->cardAttr());

                painter.drawText(cardWidth + 5, y + 10 + fmHeight * 3,
                                 adToString(card->atk) + tr("/") +
                                 adToString(card->def) + ot);
            }
            else if(card->type & (Card::TYPE_SPELL | Card::TYPE_TRAP))
            {
                painter.drawText(cardWidth + 5, y + 5 + fmHeight * 2,
                                 card->cardType());
                painter.drawText(cardWidth + 5, y + 10 + fmHeight * 3, ot);
            }
        }
        items.swap(newItems);
    }
    if(needRefreshId)
    {
        refreshCurrentId();
        needRefreshId = false;
    }
    refresh();
}

void CardsList::mousePressEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton)
    {
        startPos = event->pos();
    }
    else if(event->buttons() & Qt::MiddleButton)
    {
        int index = itemAt(event->pos());
        if(index >= 0)
        {
            emit details(ls[index]);
        }
    }
    QWidget::mousePressEvent(event);
}

int CardsList::itemAt(const QPoint &_pos)
{
    for(int i = 0; i < cardsPerColumn; i++)
    {
        if(i + pos >= ls.size())
        {
            break;
        }
        int index = i +  pos;

        auto &item = items.find(index).value();

        if(_pos.y() >= item.getPos().y() &&
                _pos.y() <= item.getPos().y() + cardSize.height() &&
                _pos.x() >= 0 && _pos.x() < width())
        {
            return index;
        }
    }
    return -1;
}

void CardsList::refreshCurrentId()
{
    int index = itemAt(point);
    if(index != -1)
    {
        int id = ls[index];
        if(currentCardId != id)
        {
            currentCardId = id;
            emit currentIdChanged(ls[index]);
        }
    }
}

void CardsList::mouseMoveEvent(QMouseEvent *event)
{
    int index = itemAt(event->pos());
    if(index != -1)
    {
        int id = ls[index];
        if(currentCardId != id)
        {
            currentCardId = id;
            emit currentIdChanged(ls[index]);
        }
        needRefreshId = false;
    }

    if(event->buttons() & Qt::LeftButton)
    {
        int dist = (event->pos() - startPos).manhattanLength();
        if(dist >= QApplication::startDragDistance() && index != -1)
        {
            current = -1;
            startDrag(index);
        }
    }
    else if(current != index)
    {
        current = index;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void CardsList::startDrag(int index)
{
    if(index >= ls.size())
    {
        return;
    }
    QMimeData *mimedata = new QMimeData;
    int id = ls[index];
    mimedata->setText(QString::number(id));
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimedata);
    auto &item = items.find(index).value();
    drag->setPixmap(item.getPixmap()->scaled(cardSize));
    drag->setHotSpot(QPoint(drag->pixmap().width() / 2,
                            drag->pixmap().height() / 2));
    moved = false;
    drag->exec(Qt::MoveAction);
}

void CardsList::dragEnterEvent(QDragEnterEvent *event)
{
    QObject *src = event->source();

    if(src)
    {
        event->accept();
    }
}

void CardsList::dragMoveEvent(QDragMoveEvent *event)
{
    QObject *src = event->source();
    if(src)
    {
        event->accept();
    }
}

void CardsList::dropEvent(QDropEvent *event)
{
    moved = true;
    event->accept();
}

void CardsList::setPos(int _pos)
{
    int max = ls.size() - cardsPerColumn;
    max = max > 0 ? max : 0;
    if(pos > max)
    {
        pos = max;
    }
    if(_pos >= 0 && _pos <= max)
    {
        pos = _pos;
        update();
    }
    if(sb)
    {
        sb->setValue(pos);
    }
}

void CardsList::setCards(QSharedPointer<QVector<int> > cards)
{
    ls.swap(*cards.data());
    ls.squeeze();

    setPos(0);

    emit sizeChanged(ls.size());
    refresh();
}

void CardsList::checkLeave()
{
    int i = itemAt(mapFromGlobal(QCursor::pos()));
    if(i != current)
    {
        current = i;
        update();
    }
}

CardsList::~CardsList()
{

}

CardsListView::CardsListView(QWidget *parent, bool &moved)
    : QWidget(parent)
{

    cl = new CardsList(nullptr, moved);
    auto hbox = new QHBoxLayout;
    auto vbox = new QVBoxLayout;
    hbox->addWidget(cl);
    connect(cl, SIGNAL(currentIdChanged(int)), this, SLOT(changeId(int)));
    auto sb = new QScrollBar;
    hbox->addWidget(sb);

    auto label = new DeckSizeLabel(config->getStr("label", "number", "数目"));
    label->setAlignment(Qt::AlignRight);
    vbox->addWidget(label);

    cl->setScrollBar(sb);

    vbox->addLayout(hbox, 1);
    connect(cl, SIGNAL(sizeChanged(int)), label, SLOT(changeSize(int)));
    connect(cl, SIGNAL(clickId(int)), this, SLOT(idClicked(int)));
    connect(cl, SIGNAL(details(int)), this, SIGNAL(details(int)));
    setLayout(vbox);
}
