







































#ifndef nsWinUtils_h_
#define nsWinUtils_h_

#include "Accessible2.h"

#include "nsIArray.h"

class nsWinUtils
{
public:
  



  static HRESULT ConvertToIA2Array(nsIArray *aCollection,
                                   IUnknown ***aAccessibles, long *aCount);
};

#endif

