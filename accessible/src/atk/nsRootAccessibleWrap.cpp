








































#include "nsMai.h"
#include "nsRootAccessibleWrap.h"

nsNativeRootAccessibleWrap::nsNativeRootAccessibleWrap(AtkObject *aAccessible):
    nsRootAccessible(nsnull, nsnull)
{
    g_object_ref(aAccessible);
    nsAccessibleWrap::mAtkObject = aAccessible;
}
