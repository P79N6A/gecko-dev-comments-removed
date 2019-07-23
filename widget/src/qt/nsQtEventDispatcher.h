






































#ifndef NSQTEVENTDISPATCHER_H
#define NSQTEVENTDISPATCHER_H

#include <qobject.h>

class nsCommonWidget;
class QEvent;
class QWidget;

class nsQtEventDispatcher : public QObject
{
    Q_OBJECT
public:
    nsQtEventDispatcher(nsCommonWidget *receiver, QWidget *watchedObject,
                        const char *name=0, bool justPaint = false);

protected:
    bool eventFilter(QObject *, QEvent *);

protected:
    nsCommonWidget *mReceiver;
    bool m_paint;
};

#endif
