




#ifndef _nsIStatefulFrame_h
#define _nsIStatefulFrame_h

#include "nsISupports.h"

class nsPresContext;
class nsPresState;

#define NS_ISTATEFULFRAME_IID \
{ 0x25c232cf, 0x40ba, 0x4394, \
 { 0x84, 0xe4, 0x73, 0xa2, 0xf7, 0x4d, 0x8b, 0x64 } }

class nsIStatefulFrame : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTATEFULFRAME_IID)

  
  
  
  enum SpecialStateID {eNoID=0, eDocumentScrollState};

  
  
  
  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState) = 0;

  
  NS_IMETHOD RestoreState(nsPresState* aState) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStatefulFrame, NS_ISTATEFULFRAME_IID)

#endif 
