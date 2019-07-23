





































#ifndef __NS_ISVGTEXTCONTENTMETRICS_H__
#define __NS_ISVGTEXTCONTENTMETRICS_H__

#include "nsQueryFrame.h"

class nsIDOMSVGRect;
class nsIDOMSVGPoint;




class nsISVGTextContentMetrics
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsISVGTextContentMetrics)
  
  NS_IMETHOD GetNumberOfChars(PRInt32 *_retval)=0;
  NS_IMETHOD GetComputedTextLength(float *_retval)=0;
  NS_IMETHOD GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval)=0;
  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)=0;
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval)=0;
  NS_IMETHOD GetCharNumAtPosition(nsIDOMSVGPoint *point, PRInt32 *_retval)=0;
};

#endif 
