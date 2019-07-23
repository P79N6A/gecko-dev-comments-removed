





































#ifndef __NS_ISVGENUM_H__
#define __NS_ISVGENUM_H__

#include "nsISupports.h"





#define NS_ISVGENUM_IID \
{ 0x6bb710c5, 0xa18c, 0x45fc, { 0xa4, 0x12, 0xf0, 0x42, 0xba, 0xe4, 0xda, 0x2d } }

class nsISVGEnum : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGENUM_IID)

  NS_IMETHOD GetIntegerValue(PRUint16 &value)=0;
  NS_IMETHOD SetIntegerValue(PRUint16 value)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGEnum, NS_ISVGENUM_IID)

#endif 

