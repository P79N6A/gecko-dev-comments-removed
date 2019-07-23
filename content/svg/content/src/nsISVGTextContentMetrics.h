





































#ifndef __NS_ISVGTEXTCONTENTMETRICS_H__
#define __NS_ISVGTEXTCONTENTMETRICS_H__

#include "nsISupports.h"
class nsIDOMSVGRect;
class nsIDOMSVGPoint;





#define NS_ISVGTEXTCONTENTMETRICS_IID \
{ 0xcbf0a774, 0x4171, 0x4112, { 0xbd, 0x9a, 0xf4, 0x9b, 0xef, 0xc0, 0xce, 0x18 } }

class nsISVGTextContentMetrics : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGTEXTCONTENTMETRICS_IID)
  
  NS_IMETHOD GetNumberOfChars(PRInt32 *_retval)=0;
  NS_IMETHOD GetComputedTextLength(float *_retval)=0;
  NS_IMETHOD GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval)=0;
  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)=0;
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval)=0;
  NS_IMETHOD GetCharNumAtPosition(nsIDOMSVGPoint *point, PRInt32 *_retval)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGTextContentMetrics,
                              NS_ISVGTEXTCONTENTMETRICS_IID)

#endif 
