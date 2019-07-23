










































#ifndef _nsILayoutHistoryState_h
#define _nsILayoutHistoryState_h

#include "nsISupports.h"
#include "nsStringFwd.h"

class nsPresState;

#define NS_ILAYOUTHISTORYSTATE_IID \
{ 0x99003f0f, 0x7ade, 0x44a1, \
 { 0x81, 0x74, 0xe3, 0x6a, 0xa5, 0xbb, 0x6b, 0x10 } }

class nsILayoutHistoryState : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILAYOUTHISTORYSTATE_IID)

  





  NS_IMETHOD AddState(const nsCString& aKey, nsPresState* aState) = 0;

  


  NS_IMETHOD GetState(const nsCString& aKey, nsPresState** aState) = 0;

  


  NS_IMETHOD RemoveState(const nsCString& aKey) = 0;

  


  NS_IMETHOD_(PRBool) HasStates() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILayoutHistoryState,
                              NS_ILAYOUTHISTORYSTATE_IID)

nsresult
NS_NewLayoutHistoryState(nsILayoutHistoryState** aState);

#endif 

