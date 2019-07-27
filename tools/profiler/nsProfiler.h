




#ifndef _NSPROFILER_H_
#define _NSPROFILER_H_

#include "nsIProfiler.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

class nsProfiler final : public nsIProfiler, public nsIObserver
{
public:
    nsProfiler();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIPROFILER

    nsresult Init();
private:
    ~nsProfiler();
    bool mLockedForPrivateBrowsing;
};

#endif 

