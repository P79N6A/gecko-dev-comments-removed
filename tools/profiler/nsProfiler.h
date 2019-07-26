




#ifndef _NSPROFILER_H_
#define _NSPROFILER_H_

#include "nsIProfiler.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

class nsProfiler MOZ_FINAL : public nsIProfiler, public nsIObserver
{
public:
    nsProfiler();
    ~nsProfiler();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIPROFILER

    nsresult Init();
private:
    bool mLockedForPrivateBrowsing;
};

#endif 

