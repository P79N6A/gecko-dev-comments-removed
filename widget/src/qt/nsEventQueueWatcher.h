






































#ifndef NSEVENTQUEUEWATCHER_H
#define NSEVENTQUEUEWATCHER_H

#include <qobject.h>
#include <qshared.h>

#include "prenv.h"

class nsIEventQueue;
class QSocketNotifier;






class nsEventQueueWatcher : public QObject,
                            public QShared
{
    Q_OBJECT
public:
    nsEventQueueWatcher(nsIEventQueue *equeue, QObject *parent, const char *name=0);
    ~nsEventQueueWatcher();

public slots:
    
    
    
    void DataReceived();

private:
    nsIEventQueue   *mEventQueue;
    QSocketNotifier *mNotifier;
    PRUint32         mRefCnt;
};

#endif
