







































#include "nsMai.h"
#include "nsRootAccessibleWrap.h"

nsNativeRootAccessibleWrap::nsNativeRootAccessibleWrap(AtkObject *aAccessible):
    nsRootAccessible(nsnull, nsnull, nsnull)
{
    g_object_ref(aAccessible);
    mAtkObject = aAccessible;
}

nsNativeRootAccessibleWrap::~nsNativeRootAccessibleWrap()
{
    g_object_unref(mAtkObject);
    mAtkObject = nsnull;
}
