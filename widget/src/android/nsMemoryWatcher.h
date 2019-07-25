




































#ifndef nsMemoryWatcher_h__
#define nsMemoryWatcher_h__

#include <stdio.h>
#include <prtime.h>
#include <prinrval.h>
#include <nsCOMPtr.h>
#include <nsITimer.h>

class nsMemoryWatcher : public nsITimerCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITIMERCALLBACK

    nsMemoryWatcher();
    virtual ~nsMemoryWatcher();

    void StartWatching();
    void StopWatching();

private:
    long mTimerInterval;
    long mLowWaterMark;
    long mHighWaterMark;
    PRIntervalTime mLastLowNotification;
    PRIntervalTime mLastHighNotification;
    nsCOMPtr<nsITimer> mTimer;
    FILE* mMemInfoFile;
};

#endif 
