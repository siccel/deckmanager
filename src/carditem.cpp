#include "carditem.h"
#include "expansions.h"
#include "card.h"
#include <QDebug>
#include <QFile>
#include <QMutex>

CardItem::Cache CardItem::m_large;
CardItem::Cache CardItem::m_small;
static QString bigPics = "pics/";
static QString smallPics = "pics/thumbnail/";
static QString unknown = "textures/unknown.jpg";

static QSharedPointer<QPixmap> readPic(QString path)
{
    auto p = QSharedPointer<QPixmap>::create(path, "1");
    if(!p || p->width() == 0)
    {
        QByteArray arr = Expansions::inst().open(path);
        p->loadFromData(arr, "1");
    }
    return p;
}

CardItem::CardItem(quint32 _id, bool small)
    : m_id(_id)
{
    auto &thePool = small ? m_small : m_large;
    auto it = thePool.find(m_id);
    if(it == thePool.end() || it.value().isNull())
    {
        if(small)
        {
            QString path = smallPics + QString::number(m_id) + ".jpg";

            m_pixmap = readPic(path);

            if(m_pixmap->width() == 0)
            {
                path = bigPics + QString::number(m_id) + ".jpg";
                m_pixmap = readPic(path);
            }
        }
        else
        {
            QString path = bigPics + QString::number(m_id) + ".jpg";
            m_pixmap = readPic(path);


            if(m_pixmap->width() == 0)
            {
                path = smallPics + QString::number(m_id) + ".jpg";
                m_pixmap = readPic(path);
            }

        }

        if(m_pixmap->width() == 0)
        {
            QString path = unknown;
            m_pixmap = readPic(path);
        }

        thePool.insert(m_id, m_pixmap.toWeakRef());
    }
    else
    {
        m_pixmap = it.value().toStrongRef();
    }
}

bool idCompare(quint32 a, quint32 b)
{
    auto ocard1 = CardManager::inst().getCard(a), ocard2 = CardManager::inst().getCard(b);
    if(ocard1 && ocard2)
    {
        Card &ca = **ocard1, &cb = **ocard2;
        int ta = ca.type & 7, tb = cb.type & 7;
        if(ta != tb)
        {
            return ta < tb;
        }
        else if(ca.type != cb.type)
        {
            return ca.type < cb.type;
        }
        else if(ca.type & Const::TYPE_MONSTER)
        {
            if(ca.level != cb.level)
            {
                return ca.level >= cb.level;
            }

            if(ca.atk != cb.atk)
            {
                return ca.atk >= cb.atk;
            }
            if(ca.def != cb.def)
            {
                return ca.def >= cb.def;
            }
            return ca.id < cb.id;
        }
        else
        {
            return ca.id < cb.id;
        }
    }
    else
    {
        return a < b;
    }
}


bool itemCompare(CardItem &a, CardItem &b)
{
    return idCompare(a.getId(), b.getId());
}
