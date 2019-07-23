





































#ifndef __NS_ISVGLENGTH_H__
#define __NS_ISVGLENGTH_H__

#include "nsIDOMSVGLength.h"
#include "nsWeakPtr.h"





#define NS_ISVGLENGTH_IID \
{ 0xdb02fd38, 0x3c77, 0x4c52, { 0x8d, 0xbd, 0xc0, 0xa4, 0x7f, 0x9d, 0xed, 0xad } }

class nsISVGLength : public nsIDOMSVGLength
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGLENGTH_IID)

  NS_IMETHOD SetContext(nsIWeakReference *aContext, PRUint8 aCtxType)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGLength, NS_ISVGLENGTH_IID)

#endif 
