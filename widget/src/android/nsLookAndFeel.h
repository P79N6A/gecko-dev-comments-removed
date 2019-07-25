





































#ifndef __nsLookAndFeel
#define __nsLookAndFeel

#include "nsXPLookAndFeel.h"
#include "AndroidBridge.h"

class nsLookAndFeel: public nsXPLookAndFeel
{
public:
    nsLookAndFeel();
    virtual ~nsLookAndFeel();

    nsresult NativeGetColor(const nsColorID aID, nscolor &aColor);
    NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
    NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);

protected:
    static PRBool mInitialized;
    static mozilla::AndroidSystemColors mSystemColors;

    nsresult GetSystemColors();
    nsresult CallRemoteGetSystemColors();
};

#endif
