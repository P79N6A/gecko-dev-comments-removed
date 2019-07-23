






































#ifndef nsIFrameUtil_h___
#define nsIFrameUtil_h___

#include <stdio.h>
#include "nsISupports.h"


#define NS_IFRAME_UTIL_IID \
 { 0xa6cf90d6, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}




class nsIFrameUtil : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAME_UTIL_IID)
  







  NS_IMETHOD CompareRegressionData(FILE* aFile1, FILE* aFile2,PRInt32 aRegressionOuput) = 0;

  





  NS_IMETHOD DumpRegressionData(FILE* aInputFile, FILE* aOutputFile) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameUtil, NS_IFRAME_UTIL_IID)

#endif 
