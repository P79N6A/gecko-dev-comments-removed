






































#include "nsEventQueueWatcher.h"

#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"

#include <qsocketnotifier.h>

nsEventQueueWatcher::nsEventQueueWatcher(nsIEventQueue *equeue, QObject *parent, const char *name)
    : QObject( parent, name ),
      mEventQueue(equeue)
{
    NS_IF_ADDREF(mEventQueue);

    Q_ASSERT(mEventQueue);

    mNotifier = new QSocketNotifier(mEventQueue->GetEventQueueSelectFD(),
                                    QSocketNotifier::Read, this);
    connect(mNotifier, SIGNAL(activated(int)),
            this, SLOT(DataReceived()) );
}

nsEventQueueWatcher::~nsEventQueueWatcher()
{
    delete mNotifier;
    NS_IF_RELEASE(mEventQueue);
}

void nsEventQueueWatcher::DataReceived()
{
    if (mEventQueue)
        mEventQueue->ProcessPendingEvents();
}

