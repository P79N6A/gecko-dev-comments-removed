




































#ifndef nsIPrivateTextRange_h__
#define nsIPrivateTextRange_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#define NS_IPRIVATETEXTRANGE_IID \
{0xb471ab41, 0x2a79, 0x11d3, \
{ 0x9e, 0xa4, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b } } 

class nsIPrivateTextRange : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATETEXTRANGE_IID)

  
  
  enum {
    TEXTRANGE_CARETPOSITION = 1,
    TEXTRANGE_RAWINPUT = 2,
    TEXTRANGE_SELECTEDRAWTEXT = 3,
    TEXTRANGE_CONVERTEDTEXT = 4,
    TEXTRANGE_SELECTEDCONVERTEDTEXT = 5
  };

  NS_IMETHOD    GetRangeStart(PRUint16* aRangeStart)=0;
  NS_IMETHOD    SetRangeStart(PRUint16 aRangeStart)=0;

  NS_IMETHOD    GetRangeEnd(PRUint16* aRangeEnd)=0;
  NS_IMETHOD    SetRangeEnd(PRUint16 aRangeEnd)=0;

  NS_IMETHOD    GetRangeType(PRUint16* aRangeType)=0;
  NS_IMETHOD    SetRangeType(PRUint16 aRangeType)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateTextRange, NS_IPRIVATETEXTRANGE_IID)

#define NS_IPRIVATETEXTRANGELIST_IID \
{0xb5a04b19, 0xed33, 0x4cd0, \
{0x82, 0xa8, 0xb7, 0x00, 0x83, 0xef, 0xc4, 0x91}}

class nsIPrivateTextRangeList : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATETEXTRANGELIST_IID)

  NS_IMETHOD_(PRUint16) GetLength()=0;
  NS_IMETHOD_(already_AddRefed<nsIPrivateTextRange>) Item(PRUint16 aIndex)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateTextRangeList,
                              NS_IPRIVATETEXTRANGELIST_IID)

#endif 
