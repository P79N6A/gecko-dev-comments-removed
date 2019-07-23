




































#ifndef _nsIPlainTextSink_h__
#define _nsIPlainTextSink_h__

#include "nsISupports.h"
#include "nsAString.h"

#define NS_PLAINTEXTSINK_CONTRACTID "@mozilla.org/layout/plaintextsink;1"


#define NS_IHTMLTOTEXTSINK_IID_STR "b12b5643-07cb-401e-aabb-64b2dcd2717f"

#define NS_IHTMLTOTEXTSINK_IID \
  {0xb12b5643, 0x07cb, 0x401e, \
    { 0xaa, 0xbb, 0x64, 0xb2, 0xdc, 0xd2, 0x71, 0x7f }}


class nsIHTMLToTextSink : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLTOTEXTSINK_IID)

  NS_IMETHOD Initialize(nsAString* aOutString,
                        PRUint32 aFlags, PRUint32 aWrapCol) = 0;
     
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLToTextSink, NS_IHTMLTOTEXTSINK_IID)

#endif
