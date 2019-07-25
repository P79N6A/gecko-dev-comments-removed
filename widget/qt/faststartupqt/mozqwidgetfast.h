





































#ifndef MOZQWIDGETFAST_H
#define MOZQWIDGETFAST_H

#include <QtCore/QObject>
#include "moziqwidget.h"

class MozQWidgetFast : public IMozQWidget
{
public:
    MozQWidgetFast(nsWindow* aReceiver, QGraphicsItem *aParent);
    ~MozQWidgetFast() {}

protected:
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

private:
    QPixmap mToolbar;
    QPixmap mIcon;
    QString mUrl;
};

#endif
