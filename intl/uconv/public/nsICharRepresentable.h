




































#ifndef nsICharRepresentable_h__
#define nsICharRepresentable_h__

#include "nscore.h"
#include "nsISupports.h"


#define NS_ICHARREPRESENTABLE_IID \
{ 0xa4d9a521, 0x185a, 0x11d3, { 0xb3, 0xbd, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }






#define IS_REPRESENTABLE(info, c) (((info)[(c) >> 5] >> ((c) & 0x1f)) & 1L)
#define SET_REPRESENTABLE(info, c)  (info)[(c) >> 5] |= (1L << ((c) & 0x1f))
#define CLEAR_REPRESENTABLE(info, c)  (info)[(c) >> 5] &= (~(1L << ((c) & 0x1f)))


#define UCS2_MAP_LEN 2048



class nsICharRepresentable : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHARREPRESENTABLE_IID)

  NS_IMETHOD FillInfo(PRUint32* aInfo) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICharRepresentable, NS_ICHARREPRESENTABLE_IID)

#endif 
