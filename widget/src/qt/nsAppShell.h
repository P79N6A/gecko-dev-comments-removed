






































#ifndef NSAPPSHELL_H
#define NSAPPSHELL_H

#include "nsIAppShell.h"
#include "nsIEventQueue.h"
#include "nsCOMPtr.h"
#include "prenv.h"

#include <qintdict.h>

class nsEventQueueWatcher;








class nsAppShell : public nsIAppShell
{
public:
    nsAppShell();
    ~nsAppShell();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAPPSHELL

private:
    void AddEventQueue(nsIEventQueue *equeue);
    void RemoveEventQueue(nsIEventQueue *equeue);

private:
    nsCOMPtr<nsIEventQueue>       mEventQueue;
    PRInt32                       mID;
    QIntDict<nsEventQueueWatcher> mQueueDict;
};

#endif
