





































#ifndef nsIFramesetElement_h___
#define nsIFramesetElement_h___

#include "nsISupports.h"


#define NS_IFRAMESETELEMENT_IID \
{ 0xeefe0fe5, 0x44ac, 0x4d7f, \
  { 0xa7, 0x51, 0xf4, 0xaa, 0x5f, 0x22, 0xb0, 0xbf } }





enum nsFramesetUnit {
  eFramesetUnit_Fixed = 0,
  eFramesetUnit_Percent,
  eFramesetUnit_Relative
};





struct nsFramesetSpec {
  nsFramesetUnit mUnit;
  nscoord        mValue;
};





class nsIFrameSetElement : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMESETELEMENT_IID)

  







  NS_IMETHOD GetRowSpec(PRInt32 *aNumValues, const nsFramesetSpec** aSpecs) = 0;

  







  NS_IMETHOD GetColSpec(PRInt32 *aNumValues, const nsFramesetSpec** aSpecs) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameSetElement, NS_IFRAMESETELEMENT_IID)

#endif 
