




#ifndef _NSPROFILER_H_
#define _NSPROFILER_H_

#include "nsIProfiler.h"
#include "mozilla/Attributes.h"

class nsProfiler MOZ_FINAL : public nsIProfiler
{
public:
    nsProfiler();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIPROFILER
};

#endif

