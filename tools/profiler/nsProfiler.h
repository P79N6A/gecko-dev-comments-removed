



































#ifndef _NSPROFILER_H_
#define _NSPROFILER_H_

#include "nsIProfiler.h"

class nsProfiler : public nsIProfiler
{
public:
    nsProfiler();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIPROFILER
};

#endif

