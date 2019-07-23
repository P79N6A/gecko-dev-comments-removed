





































#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"

class nsLookAndFeel: public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    nsresult NativeGetColor(const nsColorID aID, nscolor &aColor);
    NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
    NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);
};

#endif
