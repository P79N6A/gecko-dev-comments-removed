





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include <glib.h>
#include "nsBaseAppShell.h"
#include "nsCOMPtr.h"

class nsAppShell : public nsBaseAppShell {
public:
    nsAppShell()
        : mTag(0), mRunningMain(0)
    {
        mPipeFDs[0] = mPipeFDs[1] = 0;
    }

    
    nsresult Init();
    virtual void ScheduleNativeEventCallback();
    virtual PRBool ProcessNextNativeEvent(PRBool mayWait);

private:
    virtual ~nsAppShell();

    static gboolean EventProcessorCallback(GIOChannel *source,
                                           GIOCondition condition,
                                           gpointer data);

    int mPipeFDs[2];
    PRUintn mTag;
    PRInt32 mRunningMain;
};

#endif 
